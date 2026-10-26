#define NOMINMAX
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <sstream>
#include <windows.h>
#include <set>
#include <unordered_map>
#include <iomanip>
#include <limits>

using namespace std;
using Clock = chrono::steady_clock;

// Convert text to lowercase safely.
string toLowerCase(string text)
{
    transform(text.begin(), text.end(), text.begin(),
              [](unsigned char c)
              {
                  return static_cast<char>(tolower(c));
              });
    return text;
}

// -------------------- NAIVE / ITERATIVE SEARCH --------------------
bool naiveSearch(const string& text, const string& pattern)
{
    if (pattern.empty())
        return true;

    if (pattern.size() > text.size())
        return false;

    for (size_t i = 0; i <= text.size() - pattern.size(); ++i)
    {
        size_t j = 0;

        while (j < pattern.size() && text[i + j] == pattern[j])
            ++j;

        if (j == pattern.size())
            return true;
    }

    return false;
}

// -------------------- KMP SEARCH --------------------
vector<int> buildLPS(const string& pattern)
{
    vector<int> lps(pattern.size(), 0);
    size_t len = 0;
    size_t i = 1;

    while (i < pattern.size())
    {
        if (pattern[i] == pattern[len])
        {
            lps[i] = static_cast<int>(++len);
            ++i;
        }
        else if (len != 0)
        {
            len = static_cast<size_t>(lps[len - 1]);
        }
        else
        {
            lps[i] = 0;
            ++i;
        }
    }

    return lps;
}

bool kmpSearchPrepared(const string& text, const string& pattern,
                       const vector<int>& lps)
{
    if (pattern.empty())
        return true;

    if (pattern.size() > text.size())
        return false;

    size_t i = 0;
    size_t j = 0;

    while (i < text.size())
    {
        if (text[i] == pattern[j])
        {
            ++i;
            ++j;

            if (j == pattern.size())
                return true;
        }
        else if (j != 0)
        {
            j = static_cast<size_t>(lps[j - 1]);
        }
        else
        {
            ++i;
        }
    }

    return false;
}

bool kmpSearch(const string& text, const string& pattern)
{
    if (pattern.empty())
        return true;

    vector<int> lps = buildLPS(pattern);
    return kmpSearchPrepared(text, pattern, lps);
}

// -------------------- BOYER-MOORE BAD-CHARACTER SEARCH --------------------
unordered_map<unsigned char, int> buildBadCharTable(const string& pattern)
{
    unordered_map<unsigned char, int> lastOccurrence;
    for (size_t i = 0; i < pattern.size(); ++i)
        lastOccurrence[static_cast<unsigned char>(pattern[i])] = static_cast<int>(i);

    return lastOccurrence;
}

bool boyerMooreSearchPrepared(const string& text, const string& pattern,
                              const unordered_map<unsigned char, int>& lastOccurrence)
{
    if (pattern.empty())
        return true;

    if (pattern.size() > text.size())
        return false;

    const size_t m = pattern.size();
    size_t shift = 0;

    while (shift <= text.size() - m)
    {
        int j = static_cast<int>(m) - 1;

        while (j >= 0 && pattern[static_cast<size_t>(j)] == text[shift + static_cast<size_t>(j)])
            --j;

        if (j < 0)
            return true;

        unsigned char mismatched = static_cast<unsigned char>(text[shift + static_cast<size_t>(j)]);
        auto it = lastOccurrence.find(mismatched);

        int last = (it == lastOccurrence.end()) ? -1 : it->second;
        int moveBy = max(1, j - last);
        shift += static_cast<size_t>(moveBy);
    }

    return false;
}

bool boyerMooreSearch(const string& text, const string& pattern)
{
    if (pattern.empty())
        return true;

    auto table = buildBadCharTable(pattern);
    return boyerMooreSearchPrepared(text, pattern, table);
}

// -------------------- BENCHMARKING --------------------
struct TimingResult
{
    double preprocessingMicroseconds = 0.0;
    double searchTotalMicroseconds = 0.0;
    double totalMicroseconds = 0.0;
    double averageSearchMicroseconds = 0.0;
    bool found = false;
};

TimingResult benchmarkNaive(const string& text, const string& pattern, int repetitions)
{
    TimingResult result;
    volatile int resultSink = 0;

    auto prepStart = Clock::now();
    auto prepEnd = Clock::now();
    result.preprocessingMicroseconds =
        chrono::duration<double, micro>(prepEnd - prepStart).count();

    auto searchStart = Clock::now();
    for (int i = 0; i < repetitions; ++i)
    {
        bool found = naiveSearch(text, pattern);
        resultSink = resultSink | static_cast<int>(found);
    }
    auto searchEnd = Clock::now();

    result.searchTotalMicroseconds =
        chrono::duration<double, micro>(searchEnd - searchStart).count();
    result.totalMicroseconds = result.preprocessingMicroseconds + result.searchTotalMicroseconds;
    result.averageSearchMicroseconds = result.searchTotalMicroseconds / repetitions;
    result.found = resultSink != 0;
    return result;
}

TimingResult benchmarkKMP(const string& text, const string& pattern, int repetitions)
{
    TimingResult result;
    volatile int resultSink = 0;

    auto prepStart = Clock::now();
    vector<int> lps = buildLPS(pattern);
    auto prepEnd = Clock::now();

    result.preprocessingMicroseconds =
        chrono::duration<double, micro>(prepEnd - prepStart).count();

    auto searchStart = Clock::now();
    for (int i = 0; i < repetitions; ++i)
    {
        bool found = kmpSearchPrepared(text, pattern, lps);
        resultSink = resultSink | static_cast<int>(found);
    }
    auto searchEnd = Clock::now();

    result.searchTotalMicroseconds =
        chrono::duration<double, micro>(searchEnd - searchStart).count();
    result.totalMicroseconds = result.preprocessingMicroseconds + result.searchTotalMicroseconds;
    result.averageSearchMicroseconds = result.searchTotalMicroseconds / repetitions;
    result.found = resultSink != 0;
    return result;
}

TimingResult benchmarkBoyerMoore(const string& text, const string& pattern, int repetitions)
{
    TimingResult result;
    volatile int resultSink = 0;

    auto prepStart = Clock::now();
    auto table = buildBadCharTable(pattern);
    auto prepEnd = Clock::now();

    result.preprocessingMicroseconds =
        chrono::duration<double, micro>(prepEnd - prepStart).count();

    auto searchStart = Clock::now();
    for (int i = 0; i < repetitions; ++i)
    {
        bool found = boyerMooreSearchPrepared(text, pattern, table);
        resultSink = resultSink | static_cast<int>(found);
    }
    auto searchEnd = Clock::now();

    result.searchTotalMicroseconds =
        chrono::duration<double, micro>(searchEnd - searchStart).count();
    result.totalMicroseconds = result.preprocessingMicroseconds + result.searchTotalMicroseconds;
    result.averageSearchMicroseconds = result.searchTotalMicroseconds / repetitions;
    result.found = resultSink != 0;
    return result;
}

string jsonEscape(const string& input)
{
    ostringstream out;

    for (unsigned char c : input)
    {
        switch (c)
        {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (c < 0x20)
                {
                    const char* hex = "0123456789abcdef";
                    out << "\\u00" << hex[(c >> 4) & 0x0F] << hex[c & 0x0F];
                }
                else
                {
                    out << static_cast<char>(c);
                }
        }
    }

    return out.str();
}

void printTimingJson(const string& algorithm, const TimingResult& result)
{
    cout << "{"
         << "\"algorithm\":\"" << jsonEscape(algorithm) << "\","
         << "\"found\":" << (result.found ? "true" : "false") << ","
         << "\"preprocessing_microseconds\":" << fixed << setprecision(3)
         << result.preprocessingMicroseconds << ","
         << "\"search_total_microseconds\":" << result.searchTotalMicroseconds << ","
         << "\"average_search_microseconds\":" << result.averageSearchMicroseconds << ","
         << "\"total_microseconds\":" << result.totalMicroseconds
         << "}";
}

string makeBenchmarkText(size_t targetSize)
{
    const string chunk =
        "the quick brown fox jumps over the lazy dog. "
        "algorithm analysis string matching benchmark text. ";
    string text;
    text.reserve(targetSize);

    while (text.size() < targetSize)
    {
        size_t remaining = targetSize - text.size();
        text.append(chunk, 0, min(remaining, chunk.size()));
    }

    return text;
}

struct BenchmarkCase
{
    string name;
    string text;
    string pattern;
};

void runBenchmark()
{
    const vector<size_t> sizes = {1000, 10000, 100000};
    cout << "[\n";
    bool firstResult = true;

    for (size_t size : sizes)
    {
        // Case 1: ordinary text, absent pattern. This shows typical behaviour,
        // but may favour the simple search if its first character is uncommon.
        BenchmarkCase ordinary{
            "ordinary_absent_pattern",
            makeBenchmarkText(size),
            "xyznonexistent987"
        };

        // Case 2: repeated prefix. Naive search repeatedly rechecks the same
        // prefix, while KMP can reuse its LPS information.
        BenchmarkCase repeatedPrefix{
            "repeated_prefix_worst_case_for_naive",
            string(size, 'a'),
            "aaaaaaaaaaaaab"
        };

        // Case 3: long pattern with characters absent from the text. The
        // bad-character heuristic can skip many alignments in this scenario.
        BenchmarkCase bmFriendly{
            "bad_character_skip_friendly",
            string(size, 'a'),
            "bcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
        };

        const vector<BenchmarkCase> cases = {ordinary, repeatedPrefix, bmFriendly};

        // Reduce repetitions for larger inputs and the deliberately expensive
        // naive worst-case pattern, while keeping all algorithms equal per case.
        const int repetitions = (size == 1000) ? 100 : (size == 10000 ? 10 : 3);

        for (const auto& test : cases)
        {
            TimingResult naive = benchmarkNaive(test.text, test.pattern, repetitions);
            TimingResult kmp = benchmarkKMP(test.text, test.pattern, repetitions);
            TimingResult bm = benchmarkBoyerMoore(test.text, test.pattern, repetitions);

            if (!firstResult) cout << ",\n";
            firstResult = false;

            cout << "{"
                 << "\"text_size\":" << test.text.size() << ","
                 << "\"test_case\":\"" << jsonEscape(test.name) << "\","
                 << "\"pattern\":\"" << jsonEscape(test.pattern) << "\","
                 << "\"repetitions\":" << repetitions << ","
                 << "\"results\":[";
            printTimingJson("Iterative Search", naive);
            cout << ",";
            printTimingJson("KMP", kmp);
            cout << ",";
            printTimingJson("Boyer-Moore (bad-character)", bm);
            cout << "]}";
        }
    }

    cout << "\n]\n";
}

// -------------------- DOCUMENT UTILITIES --------------------
string readDocument(const string& filename)
{
    ifstream file(filename, ios::binary);
    if (!file)
        return "";

    ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

set<string> getUniqueWords(const string& text)
{
    set<string> words;
    string word;

    for (unsigned char c : text)
    {
        if (isalnum(c))
        {
            word += static_cast<char>(tolower(c));
        }
        else if (!word.empty())
        {
            words.insert(word);
            word.clear();
        }
    }

    if (!word.empty())
        words.insert(word);

    return words;
}

double jaccardSimilarity(const string& text1, const string& text2)
{
    set<string> words1 = getUniqueWords(text1);
    set<string> words2 = getUniqueWords(text2);

    if (words1.empty() && words2.empty())
        return 1.0;

    size_t intersectionCount = 0;
    for (const string& word : words1)
    {
        if (words2.find(word) != words2.end())
            ++intersectionCount;
    }

    size_t unionCount = words1.size() + words2.size() - intersectionCount;
    if (unionCount == 0)
        return 0.0;

    return static_cast<double>(intersectionCount) / static_cast<double>(unionCount);
}

vector<string> discoverUploadedFiles()
{
    vector<string> files;
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA("uploads\\*.txt", &findData);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                files.push_back(string("uploads\\") + findData.cFileName);
        } while (FindNextFileA(hFind, &findData));

        FindClose(hFind);
    }

    return files;
}

vector<string> getDocumentPaths()
{
    vector<string> paths = {
        "documents/sample1.txt",
        "documents/sample2.txt",
        "documents/sample3.txt"
    };

    vector<string> uploaded = discoverUploadedFiles();
    for (const string& file : uploaded)
    {
        if (find(paths.begin(), paths.end(), file) == paths.end())
            paths.push_back(file);
    }

    return paths;
}

vector<pair<string, string>> loadAvailableDocuments()
{
    vector<pair<string, string>> docs;
    for (const string& path : getDocumentPaths())
    {
        string contents = readDocument(path);
        if (!contents.empty())
            docs.push_back({path, contents});
    }
    return docs;
}

void searchDocuments(const string& keyword)
{
    if (keyword.empty())
    {
        cerr << "Error: keyword cannot be empty.\n";
        return;
    }

    const string pattern = toLowerCase(keyword);
    vector<pair<string, string>> docs = loadAvailableDocuments();

    cout << "[";
    bool first = true;

    for (const auto& doc : docs)
    {
        string text = toLowerCase(doc.second);
        if (naiveSearch(text, pattern))
        {
            if (!first) cout << ",";
            first = false;
            cout << "\"" << jsonEscape(doc.first) << "\"";
        }
    }

    cout << "]\n";
}

void interactiveMode()
{
    vector<pair<string, string>> docs = loadAvailableDocuments();

    cout << "=====================================\n";
    cout << "      DOCUMENT SEARCH TOOL\n";
    cout << "=====================================\n";

    if (docs.empty())
    {
        cout << "No readable documents found. Check the documents folder or uploads folder.\n";
    }
    else
    {
        cout << "Loaded " << docs.size() << " document(s):\n";
        for (const auto& doc : docs)
            cout << " - " << doc.first << "\n";
    }

    while (true)
    {
        cout << "\nEnter a keyword or phrase (type exit to quit): ";
        string keyword;
        getline(cin, keyword);

        if (!cin || toLowerCase(keyword) == "exit")
            break;

        if (keyword.empty())
        {
            cout << "Please enter a non-empty keyword.\n";
            continue;
        }

        string pattern = toLowerCase(keyword);
        cout << "\nSearch results:\n";
        bool foundAny = false;

        for (const auto& doc : docs)
        {
            if (naiveSearch(toLowerCase(doc.second), pattern))
            {
                cout << " [MATCH] " << doc.first << "\n";
                foundAny = true;
            }
        }

        if (!foundAny)
            cout << "No matching documents found.\n";

        if (docs.size() >= 2)
        {
            cout << "\nDocument similarity (Jaccard):\n";
            for (size_t i = 0; i < docs.size(); ++i)
            {
                for (size_t j = i + 1; j < docs.size(); ++j)
                {
                    double similarity = jaccardSimilarity(docs[i].second, docs[j].second) * 100.0;
                    cout << " " << docs[i].first << " <-> " << docs[j].first
                         << ": " << fixed << setprecision(2) << similarity << "%\n";
                }
            }
        }
        else
        {
            cout << "\nAt least two readable documents are needed for similarity comparison.\n";
        }
    }

    cout << "Exiting document search tool.\n";
}

int main(int argc, char* argv[])
{
    if (argc >= 2 && string(argv[1]) == "--benchmark")
    {
        runBenchmark();
        return 0;
    }

    if (argc >= 3 && string(argv[1]) == "--search")
    {
        string keyword = argv[2];
        for (int i = 3; i < argc; ++i)
        {
            keyword += " ";
            keyword += argv[i];
        }

        searchDocuments(keyword);
        return 0;
    }

    interactiveMode();
    return 0;
}
