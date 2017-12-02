#include <parsing>

using namespace std;

vector<string_view> split(string_view s, char separator) {
    vector<string_view> substrings;

    while (!s.empty()) {
        if (*s.data() == separator) {
            s.remove_prefix(1);
        }

        auto nextSplit = s.find(separator);

        if (nextSplit == string_view::npos) {
            break;
        }

        substrings.push_back(s.substr(0, nextSplit));
        s.remove_prefix(nextSplit);
    }

    if (!s.empty()) {
        substrings.push_back(s);
    }

    return substrings;
}