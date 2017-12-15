#include <parsing>

using namespace std;

vector<string_view> split(string_view s, char separator, bool includeSeparator) {
    vector<string_view> substrings;

    while (!s.empty()) {
        if (*s.data() == separator) {

            if (includeSeparator) {
                substrings.push_back(s.substr(0, 1));
            }

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