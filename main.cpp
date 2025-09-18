/*
    mygrep [опции] "слово" <путь или маска>
    mygrep -i -n "main" *.cpp
    mygrep -d -i "error" C:\Projects\logs\*.txt
*/

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <locale>
#include <windows.h>

using namespace std;
using namespace std::filesystem;

void parser(int argc, wchar_t* argv[], vector<wstring>& flags, vector<wstring>& files_type, wstring& sub_str, vector<wstring>& paths){
    for (int i = 1; i < argc; i++) {
        wstring s = argv[i];
        if(!s.rfind(L"-", 0)) {
            flags.push_back(s);
        } else if (!s.rfind(L"*.", 0)) {
            files_type.push_back(s.substr(1));
        }else if (sub_str.empty()) {
            sub_str = s;
        } else {
            paths.push_back(s);
        }
    }

    if (paths.size() == 0) {
        path current = current_path();
        paths.push_back(current.wstring());
    }
}

vector<wstring> get_files(const vector<wstring>& flags, const vector<wstring>& files_type, const vector<wstring>& paths) {
    vector<wstring> files;
    bool recursive = false;

    for (const auto& flag : flags) {
        if (flag == L"-d") {
            recursive = true;
            break;
        }
    }

    auto process_directory = [&](auto iterator) {
        for (const auto& entry : iterator) {
            if (!entry.is_regular_file()) {
                continue;
            }
            if (files_type.empty()) {
                files.push_back(entry.path().wstring());
            } else {
                wstring ext = entry.path().extension().wstring();
                for (const auto& type : files_type) {
                    if (ext == type) {
                        files.push_back(entry.path().wstring());
                        break;
                    }
                }
            }
        }
    };

    for (const auto& p : paths) {
        if (!exists(p) || !is_directory(p)) {
            continue;
        }
        if (recursive) {
            process_directory(recursive_directory_iterator(p, directory_options::skip_permission_denied));
        } else {
            process_directory(directory_iterator(p, directory_options::skip_permission_denied));
        }
    }

    return files;
}

void search(const wstring& str, vector<wstring>& flags, const vector<wstring>& files_type, const vector<wstring>& paths) {
    vector<wstring> files = get_files(flags, files_type, paths);
    if (files.empty()) {
        wcout << "Файлы не были наидены" << L"\n";
        return;
    }
}

int wmain(int argc, wchar_t* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    wcout.imbue(locale("en_US.UTF-8"));
    vector<wstring> flags;
    vector<wstring> paths;
    vector<wstring> files_type;
    wstring sub_str;

    parser(argc, argv, flags, files_type, sub_str, paths);

    wcout << L"Flags:\n";
    for (const auto& flag : flags) {
        wcout << flag << L"\n";
    }
    wcout << L"String:\n" << sub_str << L"\n";
    wcout << L"Files:\n";
    for (const auto& file : files_type) {
        wcout << file << L"\n";
    }
    wcout << L"Paths:\n";
    for (const auto& path : paths) {
        wcout << path << L"\n";
    }
}