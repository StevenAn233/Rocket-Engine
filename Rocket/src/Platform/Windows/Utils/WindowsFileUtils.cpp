module;

#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <wrl.h>

#ifdef RKE_DEPENDENCY_GLFW
    #include <glfw/glfw3.h>
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <glfw/glfw3native.h>
#else
    static_assert(false, u8"WindowsFileUtils: GLFW is required here!");
#endif

module FileUtils;

import Log;
import String;
import Path;

namespace {
    using namespace rke;

    String wide_to_utf8(const WCHAR* wide_str)
    {
       if(wide_str == nullptr) return {};

        const int wide_len{ static_cast<int>(wcslen(wide_str))};
        if(wide_len == 0) return {};
       
        const int size_needed{ WideCharToMultiByte
        (
            CP_UTF8,            // Code Page: UTF-8
            0,                  // Flags
            wide_str,           // Source UTF-16 string
            wide_len,           // Source length (in characters)
            NULL, 0, NULL, NULL // Buffer and size (0 to get required size)
        )};
    
        if(size_needed == 0) {
            CORE_ERROR(u8"WindowsFileUtils: Failed to convert"
                u8"wide string to UTF-8! Error: {}", GetLastError());
            return {};
        }

        CharBuffer buffer(size_needed, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide_str, wide_len,
            buffer.data(), size_needed, NULL, NULL);

        return String(str::to_char8(buffer.data()), buffer.size());
    }

    std::wstring utf8_to_wide(StringView sv)
    {
        if(sv.data() == nullptr) return {};
        int size_needed{ MultiByteToWideChar(CP_UTF8, 0,
            sv.raw_unsafe(), static_cast<int>(sv.size()), NULL, 0) };
        std::wstring wstr(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, sv.raw_unsafe(),
            static_cast<int>(sv.size()), &wstr[0], size_needed);
        return wstr;
    }
}

namespace rke
{
    std::optional<String> FileDialogs::open_file(StringView filter, const Window* window)
    {
        OPENFILENAMEW ofn{}; // A stands for ANSI
        WCHAR sz_file[260]{};
        WCHAR current_dir[256]{};
        ZeroMemory(&ofn,  sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner   = glfwGetWin32Window
            (window->get_native_window().as<GLFWwindow>());
        ofn.lpstrFile = sz_file;
        ofn.nMaxFile  = sizeof(sz_file);
        if(GetCurrentDirectoryW(256, current_dir))
            ofn.lpstrInitialDir = current_dir;
        auto wide_filter{ utf8_to_wide(filter) };
        std::replace(wide_filter.begin(), wide_filter.end(), L'|', L'\0');
        ofn.lpstrFilter  = wide_filter.c_str();

        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if(GetOpenFileNameW(&ofn) == TRUE)
            return wide_to_utf8(ofn.lpstrFile);
        return std::nullopt;
    }

    std::optional<String> FileDialogs::save_file(StringView filter, const Window* window)
    {
        OPENFILENAMEW ofn{};
        WCHAR sz_file[260]{};
    
        ZeroMemory(&ofn,  sizeof(OPENFILENAMEW));
        ofn.lStructSize = sizeof(OPENFILENAMEW);
        ofn.hwndOwner   = glfwGetWin32Window
            (window->get_native_window().as<GLFWwindow>());
        
        ofn.lpstrFile = sz_file;
        ofn.nMaxFile = sizeof(sz_file);
    
        std::wstring wide_filter{ utf8_to_wide(filter) };
        ofn.lpstrFilter = wide_filter.c_str();
    
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

        if(GetSaveFileNameW(&ofn) == TRUE)
            return wide_to_utf8(ofn.lpstrFile);
        return std::nullopt;
    }

    std::optional<String> FileDialogs::select_folder(const Window* window)
    {
        Microsoft::WRL::ComPtr<IFileOpenDialog> p_file_dialog{};
        std::optional<String> result{ std::nullopt };

        HRESULT hr{ CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE) };
        if(FAILED(hr)) return result;

        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_PPV_ARGS(&p_file_dialog));
        if(FAILED(hr))
        {
            CoUninitialize();
            return result;
        }

        DWORD dw_options{};
        hr = p_file_dialog->GetOptions(&dw_options);
        if(SUCCEEDED(hr))
        {
            // FOS_PICKFOLDERS is the most significant flag
            hr = p_file_dialog->SetOptions(dw_options | FOS_PICKFOLDERS);
        }
        if(FAILED(hr))
        {
            CoUninitialize();
            return result;
        }

        hr = p_file_dialog->Show(glfwGetWin32Window
            (window->get_native_window().as<GLFWwindow>()));

        if(SUCCEEDED(hr))
        {
            Microsoft::WRL::ComPtr<IShellItem> p_item{};
            hr = p_file_dialog->GetResult(&p_item);
            if(SUCCEEDED(hr))
            {
                PWSTR psz_file_path{ nullptr };
                hr = p_item->GetDisplayName(SIGDN_FILESYSPATH, &psz_file_path);

                if(SUCCEEDED(hr) && psz_file_path)
                {
                    result = wide_to_utf8(psz_file_path);
                    CoTaskMemFree(psz_file_path); // free
                }
            }
        }

        CoUninitialize();
        return result;
    }
}

namespace rke::file
{
    Buffer read_file_impl(const Path& file_path)
    {
        std::ifstream fin(utf8_to_wide(file_path.string()),
            std::ios::in | std::ios::binary | std::ios::ate);
        // Not for Beta: Use Unicode UTF-8 for worldwide language support!!!
        if(!fin) {
            CORE_ERROR(u8"FileUtils: Failed to open file '{}'!", file_path);
            return {};
        }

        std::streamsize size{ fin.tellg() };
        if(size == -1) {
            CORE_ERROR(u8"FileUtils: Failed to read file size: '{}'!", file_path);
            return {};
        }

        Buffer buffer{};
        buffer.resize(size);
        fin.seekg(0, std::ios::beg);
    
        fin.read(reinterpret_cast<char*>(buffer.data()), size);

        if(!fin) {
            CORE_ERROR(u8"FileUtils: Failed to read file content: '{}'!", file_path);
            return {};
        }

        CORE_INFO(u8"FileUtils: Successfully read '{}'.", file_path);
        return buffer;
    }

    bool write_file_impl(const Path& path, const char* content, Size size, bool with_endl)
    {
        std::ofstream file(utf8_to_wide(path.string()), std::ios::out | std::ios::binary);
        // Not for Beta: Use Unicode UTF-8 for worldwide language support!!!
        if(!file.is_open())
        {
            CORE_ERROR(u8"FileUtils: Failed to open file '{}' for writing.", path);
            return false;
        }
        file.write(content, size);
        if(with_endl && (size == 0 || content[size - 1] != '\n')) file.write("\n", 1);
        
        file.flush();
        file.close();
        CORE_INFO(u8"FileUtils: Wrote content to '{}' successfully.", path);
        return true;
    }

    Path get_executable_dir()
    {
        WCHAR path[MAX_PATH]{};
        GetModuleFileNameW(NULL, path, MAX_PATH);
        return Path(wide_to_utf8(path)).parent_path();
    }
}
