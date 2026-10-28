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
#include <queue>
#include <functional>
#include <map>

using namespace std;
using Clock = chrono::steady_clock;

// -------------------- TEXT UTILITIES --------------------
string toLowerCase(string text) {
    transform(text.begin(), text.end(), text.begin(),
              [](unsigned char c) { return static_cast<char>(tolower(c)); });
    return text;
}

vector<string> tokenizeWords(const string& text) {
    vector<string> words;
    string word;
    for (unsigned char c : text) {
        if (isalnum(c)) word += static_cast<char>(tolower(c));
        else if (!word.empty()) { words.push_back(word); word.clear(); }
    }
    if (!word.empty()) words.push_back(word);
    return words;
}

bool naiveSearch(const string& text, const string& pattern) {
    if (pattern.empty()) return true;
    if (pattern.size() > text.size()) return false;
    for (size_t i = 0; i <= text.size() - pattern.size(); ++i) {
        size_t j = 0;
        while (j < pattern.size() && text[i + j] == pattern[j]) ++j;
        if (j == pattern.size()) return true;
    }
    return false;
}

// -------------------- KMP SEARCH --------------------
vector<int> buildLPS(const string& pattern) {
    vector<int> lps(pattern.size(), 0);
    size_t len = 0, i = 1;
    while (i < pattern.size()) {
        if (pattern[i] == pattern[len]) lps[i++] = static_cast<int>(++len);
        else if (len != 0) len = static_cast<size_t>(lps[len - 1]);
        else lps[i++] = 0;
    }
    return lps;
}

bool kmpSearchPrepared(const string& text, const string& pattern, const vector<int>& lps) {
    if (pattern.empty()) return true;
    if (pattern.size() > text.size()) return false;
    size_t i = 0, j = 0;
    while (i < text.size()) {
        if (text[i] == pattern[j]) {
            ++i; ++j;
            if (j == pattern.size()) return true;
        } else if (j != 0) j = static_cast<size_t>(lps[j - 1]);
        else ++i;
    }
    return false;
}

bool kmpSearch(const string& text, const string& pattern) {
    if (pattern.empty()) return true;
    return kmpSearchPrepared(text, pattern, buildLPS(pattern));
}

// -------------------- BOYER-MOORE (BAD-CHARACTER RULE) --------------------
unordered_map<unsigned char, int> buildBadCharTable(const string& pattern) {
    unordered_map<unsigned char, int> lastOccurrence;
    for (size_t i = 0; i < pattern.size(); ++i)
        lastOccurrence[static_cast<unsigned char>(pattern[i])] = static_cast<int>(i);
    return lastOccurrence;
}

bool boyerMooreSearchPrepared(const string& text, const string& pattern,
                              const unordered_map<unsigned char, int>& lastOccurrence) {
    if (pattern.empty()) return true;
    if (pattern.size() > text.size()) return false;
    const size_t m = pattern.size();
    size_t shift = 0;
    while (shift <= text.size() - m) {
        int j = static_cast<int>(m) - 1;
        while (j >= 0 && pattern[static_cast<size_t>(j)] == text[shift + static_cast<size_t>(j)]) --j;
        if (j < 0) return true;
        unsigned char mismatch = static_cast<unsigned char>(text[shift + static_cast<size_t>(j)]);
        auto it = lastOccurrence.find(mismatch);
        int last = (it == lastOccurrence.end()) ? -1 : it->second;
        shift += static_cast<size_t>(max(1, j - last));
    }
    return false;
}

bool boyerMooreSearch(const string& text, const string& pattern) {
    if (pattern.empty()) return true;
    return boyerMooreSearchPrepared(text, pattern, buildBadCharTable(pattern));
}

// -------------------- BENCHMARKING --------------------
struct TimingResult {
    double preprocessingMicroseconds = 0.0;
    double searchTotalMicroseconds = 0.0;
    double totalMicroseconds = 0.0;
    double averageSearchMicroseconds = 0.0;
    bool found = false;
};

TimingResult benchmarkNaive(const string& text, const string& pattern, int repetitions) {
    TimingResult result;
    volatile int resultSink = 0;
    auto prepStart = Clock::now();
    auto prepEnd = Clock::now();
    result.preprocessingMicroseconds = chrono::duration<double, micro>(prepEnd - prepStart).count();
    auto start = Clock::now();
    for (int i = 0; i < repetitions; ++i) resultSink = resultSink | static_cast<int>(naiveSearch(text, pattern));
    auto end = Clock::now();
    result.searchTotalMicroseconds = chrono::duration<double, micro>(end - start).count();
    result.totalMicroseconds = result.preprocessingMicroseconds + result.searchTotalMicroseconds;
    result.averageSearchMicroseconds = result.searchTotalMicroseconds / repetitions;
    result.found = resultSink != 0;
    return result;
}

TimingResult benchmarkKMP(const string& text, const string& pattern, int repetitions) {
    TimingResult result;
    volatile int resultSink = 0;
    auto prepStart = Clock::now();
    vector<int> lps = buildLPS(pattern);
    auto prepEnd = Clock::now();
    result.preprocessingMicroseconds = chrono::duration<double, micro>(prepEnd - prepStart).count();
    auto start = Clock::now();
    for (int i = 0; i < repetitions; ++i) resultSink = resultSink | static_cast<int>(kmpSearchPrepared(text, pattern, lps));
    auto end = Clock::now();
    result.searchTotalMicroseconds = chrono::duration<double, micro>(end - start).count();
    result.totalMicroseconds = result.preprocessingMicroseconds + result.searchTotalMicroseconds;
    result.averageSearchMicroseconds = result.searchTotalMicroseconds / repetitions;
    result.found = resultSink != 0;
    return result;
}

TimingResult benchmarkBoyerMoore(const string& text, const string& pattern, int repetitions) {
    TimingResult result;
    volatile int resultSink = 0;
    auto prepStart = Clock::now();
    auto table = buildBadCharTable(pattern);
    auto prepEnd = Clock::now();
    result.preprocessingMicroseconds = chrono::duration<double, micro>(prepEnd - prepStart).count();
    auto start = Clock::now();
    for (int i = 0; i < repetitions; ++i)
        resultSink = resultSink | static_cast<int>(boyerMooreSearchPrepared(text, pattern, table));
    auto end = Clock::now();
    result.searchTotalMicroseconds = chrono::duration<double, micro>(end - start).count();
    result.totalMicroseconds = result.preprocessingMicroseconds + result.searchTotalMicroseconds;
    result.averageSearchMicroseconds = result.searchTotalMicroseconds / repetitions;
    result.found = resultSink != 0;
    return result;
}

string jsonEscape(const string& input) {
    ostringstream out;
    for (unsigned char c : input) {
        switch (c) {
            case '"': out << "\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (c < 0x20) {
                    const char* hex = "0123456789abcdef";
                    out << "\\u00" << hex[(c >> 4) & 0x0F] << hex[c & 0x0F];
                } else out << static_cast<char>(c);
        }
    }
    return out.str();
}

void printTimingJson(const string& algorithm, const TimingResult& result) {
    cout << "{\"algorithm\":\"" << jsonEscape(algorithm) << "\","
         << "\"found\":" << (result.found ? "true" : "false") << ","
         << "\"preprocessing_microseconds\":" << fixed << setprecision(3) << result.preprocessingMicroseconds << ","
         << "\"search_total_microseconds\":" << result.searchTotalMicroseconds << ","
         << "\"average_search_microseconds\":" << result.averageSearchMicroseconds << ","
         << "\"total_microseconds\":" << result.totalMicroseconds << "}";
}

string makeBenchmarkText(size_t targetSize) {
    const string chunk = "the quick brown fox jumps over the lazy dog. algorithm analysis string matching benchmark text. ";
    string text;
    text.reserve(targetSize);
    while (text.size() < targetSize) {
        size_t remaining = targetSize - text.size();
        text.append(chunk, 0, min(remaining, chunk.size()));
    }
    return text;
}

struct BenchmarkCase { string name; string text; string pattern; };

void runBenchmark() {
    const vector<size_t> sizes = {1000, 10000, 100000};
    cout << "[\n";
    bool firstResult = true;
    for (size_t size : sizes) {
        vector<BenchmarkCase> cases = {
            {"ordinary_absent_pattern", makeBenchmarkText(size), "xyznonexistent987"},
            {"repeated_prefix_worst_case_for_naive", string(size, 'a'), "aaaaaaaaaaaaab"},
            {"bad_character_skip_friendly", string(size, 'a'), "bcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"}
        };
        const int repetitions = (size == 1000) ? 100 : (size == 10000 ? 10 : 3);
        for (const auto& test : cases) {
            TimingResult naive = benchmarkNaive(test.text, test.pattern, repetitions);
            TimingResult kmp = benchmarkKMP(test.text, test.pattern, repetitions);
            TimingResult bm = benchmarkBoyerMoore(test.text, test.pattern, repetitions);
            if (!firstResult) cout << ",\n";
            firstResult = false;
            cout << "{\"text_size\":" << test.text.size()
                 << ",\"test_case\":\"" << jsonEscape(test.name)
                 << "\",\"pattern\":\"" << jsonEscape(test.pattern)
                 << "\",\"repetitions\":" << repetitions << ",\"results\":[";
            printTimingJson("Iterative Search", naive); cout << ",";
            printTimingJson("KMP", kmp); cout << ",";
            printTimingJson("Boyer-Moore (bad-character)", bm);
            cout << "]}";
        }
    }
    cout << "\n]\n";
}

// -------------------- DOCUMENT UTILITIES --------------------
string readDocument(const string& filename) {
    ifstream file(filename, ios::binary);
    if (!file) return "";
    ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

set<string> getUniqueWords(const string& text) {
    vector<string> tokens = tokenizeWords(text);
    return set<string>(tokens.begin(), tokens.end());
}

double jaccardSimilarity(const string& text1, const string& text2) {
    set<string> words1 = getUniqueWords(text1);
    set<string> words2 = getUniqueWords(text2);
    if (words1.empty() && words2.empty()) return 1.0;
    size_t intersectionCount = 0;
    for (const string& word : words1)
        if (words2.find(word) != words2.end()) ++intersectionCount;
    size_t unionCount = words1.size() + words2.size() - intersectionCount;
    if (unionCount == 0) return 0.0;
    return static_cast<double>(intersectionCount) / static_cast<double>(unionCount);
}

vector<string> discoverUploadedFiles() {
    vector<string> files;
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA("uploads\\*.txt", &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                files.push_back(string("uploads\\") + findData.cFileName);
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
    return files;
}

vector<string> getDocumentPaths() {
    vector<string> paths = {"documents/sample1.txt", "documents/sample2.txt", "documents/sample3.txt"};
    vector<string> uploaded = discoverUploadedFiles();
    for (const string& file : uploaded)
        if (find(paths.begin(), paths.end(), file) == paths.end()) paths.push_back(file);
    return paths;
}

vector<pair<string, string>> loadAvailableDocuments() {
    vector<pair<string, string>> docs;
    for (const string& path : getDocumentPaths()) {
        string contents = readDocument(path);
        if (!contents.empty()) docs.push_back({path, contents});
    }
    return docs;
}

// Build an inverted index: word -> paths of documents containing that word.
unordered_map<string, vector<string>> buildInvertedIndex(const vector<pair<string, string>>& docs) {
    unordered_map<string, vector<string>> index;
    for (const auto& doc : docs) {
        set<string> wordsInDocument;
        for (const string& word : tokenizeWords(doc.second)) wordsInDocument.insert(word);
        for (const string& word : wordsInDocument) index[word].push_back(doc.first);
    }
    return index;
}

// Count case-insensitive substring occurrences for basic relevance scoring.
size_t countOccurrences(const string& text, const string& pattern) {
    if (pattern.empty()) return 0;
    size_t count = 0, pos = 0;
    while ((pos = text.find(pattern, pos)) != string::npos) {
        ++count;
        pos += pattern.size();
    }
    return count;
}

struct RankedDocument {
    string path;
    size_t score;
};

struct LowerScoreFirst {
    bool operator()(const RankedDocument& a, const RankedDocument& b) const {
        if (a.score != b.score) return a.score < b.score;
        return a.path > b.path;
    }
};

void searchDocuments(const string& keyword) {
    if (keyword.empty()) { cerr << "Error: keyword cannot be empty.\n"; return; }
    vector<pair<string, string>> docs = loadAvailableDocuments();
    string pattern = toLowerCase(keyword);

    vector<string> queryWords = tokenizeWords(keyword);
    bool singleWordQuery = (queryWords.size() == 1 &&
                            keyword.find_first_of(" \t\r\n") == string::npos &&
                            queryWords[0] == pattern);
    vector<string> candidatePaths;

    if (singleWordQuery) {
        // Inverted index narrows the candidate documents for single-word queries.
        unordered_map<string, vector<string>> index = buildInvertedIndex(docs);
        auto it = index.find(queryWords[0]);
        if (it != index.end()) candidatePaths = it->second;
    } else {
        // Keep substring matching for exact phrase/substring searches.
        for (const auto& doc : docs)
            if (naiveSearch(toLowerCase(doc.second), pattern)) candidatePaths.push_back(doc.first);
    }

    // Rank matched documents by occurrence count using a heap. The API still
    // returns a JSON array of paths, so the existing Node.js frontend stays compatible.
    priority_queue<RankedDocument, vector<RankedDocument>, LowerScoreFirst> ranked;
    for (const string& path : candidatePaths) {
        for (const auto& doc : docs) {
            if (doc.first == path) {
                size_t score = countOccurrences(toLowerCase(doc.second), pattern);
                ranked.push({path, score});
                break;
            }
        }
    }

    cout << "[";
    bool first = true;
    while (!ranked.empty()) {
        if (!first) cout << ",";
        first = false;
        cout << "\"" << jsonEscape(ranked.top().path) << "\"";
        ranked.pop();
    }
    cout << "]\n";
}

// Hash-based exact duplicate detection. A hash match is verified against the
// complete content to avoid treating a hash collision as a duplicate.
vector<pair<string, string>> findExactDuplicates(const vector<pair<string, string>>& docs) {
    unordered_map<size_t, vector<size_t>> byHash;
    vector<pair<string, string>> duplicates;
    hash<string> hasher;
    for (size_t i = 0; i < docs.size(); ++i) {
        size_t contentHash = hasher(docs[i].second);
        auto& previous = byHash[contentHash];
        for (size_t j : previous) {
            if (docs[i].second == docs[j].second)
                duplicates.push_back({docs[j].first, docs[i].first});
        }
        previous.push_back(i);
    }
    return duplicates;
}

// LCS over token sequences. Returns the number of shared tokens in order.
// The cap prevents unexpectedly huge O(n*m) memory use on very large documents.
size_t lcsTokenLength(const string& text1, const string& text2) {
    vector<string> a = tokenizeWords(text1);
    vector<string> b = tokenizeWords(text2);
    const size_t MAX_TOKENS = 3000;
    if (a.size() > MAX_TOKENS) a.resize(MAX_TOKENS);
    if (b.size() > MAX_TOKENS) b.resize(MAX_TOKENS);

    vector<size_t> previous(b.size() + 1, 0), current(b.size() + 1, 0);
    for (size_t i = 1; i <= a.size(); ++i) {
        for (size_t j = 1; j <= b.size(); ++j) {
            if (a[i - 1] == b[j - 1]) current[j] = previous[j - 1] + 1;
            else current[j] = max(previous[j], current[j - 1]);
        }
        previous.swap(current);
        fill(current.begin(), current.end(), 0);
    }
    return previous[b.size()];
}

// LCS similarity percentage: 2*LCS/(lengthA+lengthB)*100.
double lcsSimilarity(const string& text1, const string& text2) {
    vector<string> a = tokenizeWords(text1);
    vector<string> b = tokenizeWords(text2);
    if (a.empty() && b.empty()) return 100.0;
    size_t originalA = a.size(), originalB = b.size();
    size_t common = lcsTokenLength(text1, text2);
    // This score is based on the compared prefixes if either input exceeds the cap.
    size_t comparedA = min(originalA, static_cast<size_t>(3000));
    size_t comparedB = min(originalB, static_cast<size_t>(3000));
    size_t denominator = comparedA + comparedB;
    if (denominator == 0) return 0.0;
    return 200.0 * static_cast<double>(common) / static_cast<double>(denominator);
}

void printDuplicateReport() {
    vector<pair<string, string>> docs = loadAvailableDocuments();
    vector<pair<string, string>> duplicates = findExactDuplicates(docs);
    cout << "Exact duplicate pairs (hash checked against full content):\n";
    if (duplicates.empty()) cout << "No exact duplicate documents found.\n";
    for (const auto& pair : duplicates)
        cout << " - " << pair.first << " <-> " << pair.second << "\n";
}

void interactiveMode() {
    vector<pair<string, string>> docs = loadAvailableDocuments();
    cout << "=====================================\n"
         << "      DOCUMENT SEARCH TOOL\n"
         << "=====================================\n";
    if (docs.empty()) cout << "No readable documents found. Check the documents folder or uploads folder.\n";
    else {
        cout << "Loaded " << docs.size() << " document(s):\n";
        for (const auto& doc : docs) cout << " - " << doc.first << "\n";
    }
    while (true) {
        cout << "\nEnter a keyword or phrase (type exit to quit): ";
        string keyword;
        getline(cin, keyword);
        if (!cin || toLowerCase(keyword) == "exit") break;
        if (keyword.empty()) { cout << "Please enter a non-empty keyword.\n"; continue; }

        if (toLowerCase(keyword) == "duplicates") { printDuplicateReport(); continue; }

        string pattern = toLowerCase(keyword);
        vector<string> queryWords = tokenizeWords(keyword);
        bool singleWordQuery = (queryWords.size() == 1 &&
                                keyword.find_first_of(" \t\r\n") == string::npos &&
                                queryWords[0] == pattern);
        vector<string> matches;
        if (singleWordQuery) {
            auto index = buildInvertedIndex(docs);
            auto it = index.find(queryWords[0]);
            if (it != index.end()) matches = it->second;
        } else {
            for (const auto& doc : docs)
                if (naiveSearch(toLowerCase(doc.second), pattern)) matches.push_back(doc.first);
        }

        cout << "\nSearch results:\n";
        if (matches.empty()) cout << "No matching documents found.\n";
        else for (const string& path : matches) cout << " [MATCH] " << path << "\n";

        if (docs.size() >= 2) {
            cout << "\nDocument similarity (Jaccard and LCS):\n";
            for (size_t i = 0; i < docs.size(); ++i)
                for (size_t j = i + 1; j < docs.size(); ++j)
                    cout << " " << docs[i].first << " <-> " << docs[j].first
                         << " | Jaccard: " << fixed << setprecision(2)
                         << jaccardSimilarity(docs[i].second, docs[j].second) * 100.0 << "%"
                         << " | LCS: " << lcsSimilarity(docs[i].second, docs[j].second) << "%\n";
        } else cout << "\nAt least two readable documents are needed for similarity comparison.\n";
    }
    cout << "Exiting document search tool.\n";
}

int main(int argc, char* argv[]) {
    if (argc >= 2 && string(argv[1]) == "--benchmark") {
        runBenchmark();
        return 0;
    }
    if (argc >= 2 && string(argv[1]) == "--duplicates") {
        printDuplicateReport();
        return 0;
    }
    if (argc >= 3 && string(argv[1]) == "--search") {
        string keyword = argv[2];
        for (int i = 3; i < argc; ++i) { keyword += " "; keyword += argv[i]; }
        searchDocuments(keyword);
        return 0;
    }
    interactiveMode();
    return 0;
}

