export module RkDylibLoader;

import Path;
import String;
import HeapManager;
import Types;
import RkDylib;

export namespace rke
{
    class RkDylibLoader final
    {
    public:
        RkDylibLoader(Path dir, String name);
        ~RkDylibLoader();

        RkDylibLoader(const RkDylibLoader&) = delete;
        RkDylibLoader& operator=(const RkDylibLoader&) = delete;
        RkDylibLoader(RkDylibLoader&&) = default;
        RkDylibLoader& operator=(RkDylibLoader&&) = default;

        Scope<RkDylib> load_fresh();
    private:
        StringView dylib_postfix();

        Path copy_dylib_file();
        void prune_temp_files();
        void delete_temp_files();
    private:
        Path dylib_dir_{};
        String dylib_name_{};
        uint32 reload_count_{};
    };
}
