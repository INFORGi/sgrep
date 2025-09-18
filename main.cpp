/*
    sgrep [опции] "слово" <путь или маска>
    sgrep -i -n "main" *.cpp
    sgrep -d -i "error" "C:\Projects\logs\*.txt"
    sgrep "main" "C:\Projects\logs\*.txt" "C:\Projects\logs" *.cpp *.h
    sgrep "main" " " *.cpp *.h
*/

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <locale>
#include <windows.h>

using namespace std;
using namespace std::filesystem;

struct Options {
    bool recursive = false;         // -d
    bool ignore_case = false;       // -i
    bool show_line_numbers = false; // -n
};

void parser(int argc, wchar_t* argv[], Options& options, wstring& sub_str, vector<pair<wstring, vector<wstring>>>& path_types) {
    vector<wstring> global_files_type;

    for (int i = 1; i < argc; i++) {
        wstring s = argv[i];
        if (s.find(L"-") == 0) {
            if (s == L"-d")         options.recursive = true;
            else if (s == L"-i")    options.ignore_case = true;
            else if (s == L"-n")    options.show_line_numbers = true;
        } else if (sub_str.empty()) {
            sub_str = s;
        } else {
            path p(s);
            wstring normalized_path = p.lexically_normal().wstring();

            if (p.filename().wstring().find(L"*.") == 0 && p.parent_path().wstring().empty()) {
                global_files_type.push_back(p.filename().wstring().substr(1));
            } else if (p.filename().wstring().find(L"*.") == 0 && !p.parent_path().wstring().empty()) {
                path_types.emplace_back(p.parent_path().wstring(), vector<wstring>{p.extension()});
            } else {
                path_types.emplace_back(normalized_path, vector<wstring>{});
            }
        }
    }

    if (path_types.empty() && !global_files_type.empty()) {
        path_types.emplace_back(current_path().wstring(), global_files_type);
    } else {
        for (auto& [path, types] : path_types) {
            if (types.empty()) {
                types = global_files_type;
            }
        }
    }
}

vector<wstring> get_files(const Options& options, const vector<pair<wstring, vector<wstring>>>& path_types) {
    vector<wstring> files;

    auto process_directory = [&](auto iterator, const vector<wstring>& types) {
        for (const auto& entry : iterator) {
            if (!entry.is_regular_file()) continue;
            if (types.empty()) {
                files.push_back(entry.path().wstring());
            } else {
                wstring ext = entry.path().extension().wstring();
                for (const auto& type : types) {
                    if (ext == type) {
                        files.push_back(entry.path().wstring());
                        break;
                    }
                }
            }
        }
    };

    for (const auto& [p, types] : path_types) {
        if (!exists(p) || !is_directory(p)) continue;
        if (options.recursive) {
            process_directory(recursive_directory_iterator(p, directory_options::skip_permission_denied), types);
        } else {
            process_directory(directory_iterator(p, directory_options::skip_permission_denied), types);
        }
    }

    return files;
}

wstring to_lower(const wstring& input) {
    if (input.empty()) return L"";

    int len = LCMapStringEx(LOCALE_NAME_USER_DEFAULT, LCMAP_LOWERCASE, input.c_str(), input.length(), nullptr, 0, nullptr, nullptr, 0);
    if (len == 0) return input;

    vector<wchar_t> buffer(len);
    LCMapStringEx(LOCALE_NAME_USER_DEFAULT, LCMAP_LOWERCASE, input.c_str(), input.length(), buffer.data(), len, nullptr, nullptr, 0);

    return wstring(buffer.data(), len);
}

wstring read_file(const path& file_path, const wstring& str, const Options& options) {
    ifstream file(file_path);
    if (!file.is_open()) {
        wcout << L"Не удалось открыть файл: " << file_path.wstring() << endl;
        return L"";
    }

    wstring result;
    string line;
    int index = 0;

    wstring search_str = options.ignore_case ? to_lower(str) : str;

    while (getline(file, line)) {
        wstring s(line.begin(), line.end());
        index++;

        wstring s_lower = options.ignore_case ? to_lower(s) : s;
        bool found = s_lower.find(search_str) != wstring::npos;

        if (found) {
            wstring output_line = options.show_line_numbers ? to_wstring(index) + L": " + s : s;
            result += output_line + L"\n";
        }
    }

    return result.empty() ? L"" : result.substr(0, result.size() - 1);
}

void search(const wstring& str, const Options& options, const vector<pair<wstring, vector<wstring>>>& path_types) {
    vector<wstring> files = get_files(options, path_types);
    if (files.empty()) {
        wcout << L"Файлы не найдены" << endl;
        return;
    } else if (str == L" ") {
        for (const auto& file : files) {
            wcout << L"Найден файл: " << file << L"\n";
        }
    } else {
        for (const auto& file : files) {
            wcout << file << "\n" << read_file(file, str, options);
        }
    }
}

int wmain(int argc, wchar_t* argv[]) {
    char *locale = setlocale(LC_ALL, "");
    // wcout.imbue(locale(""));

    Options options;
    vector<wstring> files_type;
    wstring sub_str;
    vector<pair<wstring, vector<wstring>>> path_types;

    parser(argc, argv, options, sub_str, path_types);
    search(sub_str, options, path_types);
    
    return 0;
}