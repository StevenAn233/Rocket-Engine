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
        String temp_name_of(uint32 index);
        void prune_temp_files();
        void delete_temp_files();
    private:
        static const char8* s_temp_rep_;
    private:
        Path dylib_dir_{};
        String dylib_name_{};
        uint32 reload_count_{};
    };
}
