#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "installer_data.h"

static void set_console_utf8(void) {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
}

static void print_step(const char *msg) {
    printf("  >> %s\n", msg);
}

static void print_ok(const char *msg) {
    printf("  [ok] %s\n", msg);
}

static void print_fail(const char *msg) {
    fprintf(stderr, "  [fail] %s\n", msg);
}

static int make_dirs(const char *path) {
    char tmp[MAX_PATH];
    strncpy(tmp, path, MAX_PATH - 1);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '\\' || *p == '/') {
            *p = '\0';
            CreateDirectoryA(tmp, NULL);
            *p = '\\';
        }
    }
    return CreateDirectoryA(tmp, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

static int write_file(const char *path, const unsigned char *data, int size) {
    HANDLE h = CreateFileA(path, GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return 0;
    DWORD written;
    WriteFile(h, data, size, &written, NULL);
    CloseHandle(h);
    return (int)written == size;
}

static void normalize_path(char *path) {
    for (char *p = path; *p; p++) {
        if (*p == '/') *p = '\\';
    }
    size_t len = strlen(path);
    while (len > 1 && path[len - 1] == '\\') {
        path[--len] = '\0';
    }
}

static int path_contains(const char *path_env, const char *dir) {
    char copy[32768];
    strncpy(copy, path_env, sizeof(copy) - 1);
    char dir_norm[MAX_PATH];
    strncpy(dir_norm, dir, MAX_PATH - 1);
    normalize_path(dir_norm);

    char *token = strtok(copy, ";");
    while (token) {
        char tok_norm[MAX_PATH];
        strncpy(tok_norm, token, MAX_PATH - 1);
        normalize_path(tok_norm);
        if (_stricmp(tok_norm, dir_norm) == 0) return 1;
        token = strtok(NULL, ";");
    }
    return 0;
}

static void add_to_path(const char *bin_dir) {
    HKEY key;
    RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_READ | KEY_WRITE, &key);

    char current_path[32768] = {0};
    DWORD size = sizeof(current_path);
    DWORD type;
    RegQueryValueExA(key, "Path", NULL, &type, (LPBYTE)current_path, &size);

    if (!path_contains(current_path, bin_dir)) {
        char new_path[32768];
        if (strlen(current_path) > 0) {
            snprintf(new_path, sizeof(new_path), "%s;%s", current_path, bin_dir);
        } else {
            strncpy(new_path, bin_dir, sizeof(new_path) - 1);
        }
        RegSetValueExA(key, "Path", 0, REG_EXPAND_SZ,
                       (LPBYTE)new_path, (DWORD)(strlen(new_path) + 1));
        print_ok("Added to PATH.");
    } else {
        print_ok("Already in PATH.");
    }

    RegCloseKey(key);

    SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                        (LPARAM)"Environment", SMTO_ABORTIFHUNG, 2000, NULL);
}

static void set_env_var(const char *name, const char *value) {
    HKEY key;
    RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_WRITE, &key);
    RegSetValueExA(key, name, 0, REG_EXPAND_SZ,
                   (LPBYTE)value, (DWORD)(strlen(value) + 1));
    RegCloseKey(key);
}

int main(void) {
    set_console_utf8();

    printf("\n");
    printf("=====================================\n");
    printf("  Fluxa Installer v" FLUXA_INSTALLER_VERSION "\n");
    printf("=====================================\n");
    printf("\n");

    char local_appdata[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, local_appdata))) {
        GetEnvironmentVariableA("LOCALAPPDATA", local_appdata, MAX_PATH);
    }

    char install_root[MAX_PATH], bin_dir[MAX_PATH], lib_dir[MAX_PATH];
    snprintf(install_root, MAX_PATH, "%s\\Programs\\Fluxa",  local_appdata);
    snprintf(bin_dir,      MAX_PATH, "%s\\bin",              install_root);
    snprintf(lib_dir,      MAX_PATH, "%s\\lib",              install_root);

    print_step("Creating directories...");
    make_dirs(bin_dir);
    make_dirs(lib_dir);
    print_ok("Directories created.");

    print_step("Installing fluxa.exe...");
    char exe_dest[MAX_PATH];
    snprintf(exe_dest, MAX_PATH, "%s\\fluxa.exe", bin_dir);
    if (!write_file(exe_dest, fluxa_exe_data, fluxa_exe_size)) {
        print_fail("Failed to write fluxa.exe");
        return 1;
    }
    print_ok("fluxa.exe installed.");

    print_step("Installing standard library...");
    for (int i = 0; i < fluxa_lib_count; i++) {
        char dest[MAX_PATH];
        snprintf(dest, MAX_PATH, "%s\\%s", lib_dir, fluxa_lib_names[i]);
        if (!write_file(dest, fluxa_lib_data[i], fluxa_lib_sizes[i])) {
            print_fail(fluxa_lib_names[i]);
        }
    }
    printf("  [ok] %d library file(s) installed.\n", fluxa_lib_count);

    print_step("Adding to PATH...");
    add_to_path(bin_dir);

    print_step("Setting environment variables...");
    set_env_var("FLUXA_HOME", install_root);
    set_env_var("FLUXA_PATH", lib_dir);
    print_ok("FLUXA_HOME and FLUXA_PATH set.");

    printf("\n");
    printf("=====================================\n");
    printf("  Installation complete!\n");
    printf("=====================================\n");
    printf("\n");
    printf("  Home : %s\n", install_root);
    printf("  Bin  : %s\n", bin_dir);
    printf("  Lib  : %s\n", lib_dir);
    printf("\n");
    printf("  Open a new terminal and run:\n");
    printf("    fluxa version\n");
    printf("\n");
    printf("Press Enter to exit...\n");
    getchar();
    return 0;
}
