
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

using namespace std;

// Convert text to lowercase
string toLowerCase(string text)
{
    transform(text.begin(), text.end(), text.begin(),
              [](unsigned char c)
              {
                  return static_cast<char>(tolower(c));
              });
    return text;
}

// Linear Search
bool linearSearch(string text, string pattern)
{
    int n = text.length();
    int m = pattern.length();

    if (m == 0)
        return true;

    if (m > n)
        return false;

    for (int i = 0; i <= n - m; i++)
    {
        int j = 0;

        while (j < m && text[i + j] == pattern[j])
            j++;

        if (j == m)
            return true;
    }

    return false;
}

// Build LPS array for KMP
vector<int> buildLPS(string pattern)
{
    int m = pattern.length();
    vector<int> lps(m, 0);

    int len = 0;
    int i = 1;

    while (i < m)
    {
        if (pattern[i] == pattern[len])
        {
            len++;
            lps[i] = len;
            i++;
        }
        else
        {
            if (len != 0)
                len = lps[len - 1];
            else
            {
                lps[i] = 0;
                i++;
            }
        }
    }

    return lps;
}

// KMP Search
bool kmpSearch(string text, string pattern)
{
    int n = text.length();
    int m = pattern.length();

    if (m == 0)
        return true;

    if (m > n)
        return false;

    vector<int> lps = buildLPS(pattern);

    int i = 0;
    int j = 0;

    while (i < n)
    {
        if (text[i] == pattern[j])
        {
            i++;
            j++;
        }

        if (j == m)
            return true;
        else if (i < n && text[i] != pattern[j])
        {
            if (j != 0)
                j = lps[j - 1];
            else
                i++;
        }
    }

    return false;
}

// Boyer-Moore bad character shift
int badCharShift(string pattern, char mismatch, int j)
{
    int last = -1;

    for (int i = 0; i < j; i++)
    {
        if (pattern[i] == mismatch)
            last = i;
    }

    return max(1, j - last);
}

// Boyer-Moore Search
bool boyerMooreSearch(string text, string pattern)
{
    int n = text.length();
    int m = pattern.length();

    if (m == 0)
        return true;

    if (m > n)
        return false;

    int s = 0;

    while (s <= n - m)
    {
        int j = m - 1;

        while (j >= 0 && pattern[j] == text[s + j])
            j--;

        if (j < 0)
            return true;

        s += badCharShift(pattern, text[s + j], j);
    }

    return false;
}

// Benchmark search algorithms
long long benchmarkSearch(int algorithm, string text,
                          string pattern, int repetitions = 10000)
{
    bool result = false;

    auto start = chrono::high_resolution_clock::now();

    for (int i = 0; i < repetitions; i++)
    {
        if (algorithm == 1)
            result = linearSearch(text, pattern);
        else if (algorithm == 2)
            result = kmpSearch(text, pattern);
        else if (algorithm == 3)
            result = boyerMooreSearch(text, pattern);
    }

    auto end = chrono::high_resolution_clock::now();

    (void)result;

    return chrono::duration_cast<chrono::microseconds>(
               end - start)
        .count();
}

// Extract unique words from document text
set<string> getUniqueWords(string text)
{
    set<string> words;
    stringstream ss(text);
    string word;

    while (ss >> word)
    {
        word = toLowerCase(word);

        string cleanedWord = "";

        for (char c : word)
        {
            if (isalnum(static_cast<unsigned char>(c)))
                cleanedWord += c;
        }

        if (!cleanedWord.empty())
            words.insert(cleanedWord);
    }

    return words;
}

// Calculate Jaccard similarity
double calculateJaccardSimilarity(set<string> wordsA,
                                  set<string> wordsB)
{
    int intersection = 0;

    for (const string &word : wordsA)
    {
        if (wordsB.find(word) != wordsB.end())
            intersection++;
    }

    set<string> unionWords = wordsA;
    unionWords.insert(wordsB.begin(), wordsB.end());

    if (unionWords.empty())
        return 0.0;

    double similarity =
        (double)intersection / unionWords.size();

    return similarity * 100.0;
}

// Read complete document
string readDocument(string filename)
{
    ifstream file(filename);
    string content;
    string line;

    if (!file.is_open())
        return "";

    while (getline(file, line))
        content += line + " ";

    file.close();

    return content;
}

// Escape strings for valid JSON output
string jsonEscape(string text)
{
    string result;

    for (char c : text)
    {
        if (c == '\\')
            result += "\\\\";
        else if (c == '"')
            result += "\\\"";
        else if (c == '\n')
            result += "\\n";
        else if (c == '\r')
            result += "\\r";
        else if (c == '\t')
            result += "\\t";
        else
            result += c;
    }

    return result;
}

// Add all uploaded TXT files to the document list
void addUploadedDocuments(vector<string> &filenames)
{
    WIN32_FIND_DATAA findData;

    HANDLE hFind = FindFirstFileA(
        "uploads\\*.txt", &findData);

    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do
    {
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            string filename = "uploads/";
            filename += findData.cFileName;

            filenames.push_back(filename);
        }

    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
}

int main(int argc, char *argv[])
{
    // Predefined sample documents
    vector<string> filenames = {
        "documents/sample1.txt",
        "documents/sample2.txt",
        "documents/sample3.txt"};

    // Add uploaded documents automatically
    addUploadedDocuments(filenames);

    // API mode: benchmark algorithms on different input sizes
    if (argc >= 3 && string(argv[1]) == "--benchmark")
    {
        string keyword = "xyznonexistent987";

        string originalText = "";

        for (const string &filename : filenames)
        {
            originalText += readDocument(filename) + " ";
        }

        if (originalText.empty())
        {
            cerr << "No document content available for benchmarking.";
            return 1;
        }

        // Small, medium and large synthetic input sizes
        vector<int> inputSizes = {1000, 10000, 100000};
        vector<int> repetitions = {10000, 1000, 100};

        vector<string> sizeNames = {
            "small", "medium", "large"};

        cout << "{";
        cout << "\"keyword\":\"" << jsonEscape(keyword) << "\",";
        cout << "\"unit\":\"microseconds\",";
        cout << "\"results\":[";

        for (int i = 0; i < (int)inputSizes.size(); i++)
        {
            string benchmarkText = "";

            // Repeat existing document content to reach target size
            while (benchmarkText.size() < (size_t)inputSizes[i])
            {
                benchmarkText += originalText;
            }

            benchmarkText.resize(inputSizes[i]);

            int reps = repetitions[i];

            long long linearTime =
                benchmarkSearch(1, benchmarkText, keyword, reps);

            long long kmpTime =
                benchmarkSearch(2, benchmarkText, keyword, reps);

            long long boyerMooreTime =
                benchmarkSearch(3, benchmarkText, keyword, reps);

            if (i > 0)
                cout << ",";

            cout << "{"
                 << "\"size\":\"" << sizeNames[i] << "\","
                 << "\"characters\":" << inputSizes[i] << ","
                 << "\"repetitions\":" << reps << ","
                 << "\"linearSearch\":" << linearTime << ","
                 << "\"kmpSearch\":" << kmpTime << ","
                 << "\"boyerMooreSearch\":" << boyerMooreTime << ","
                 << "\"linearAverage\":" << (double)linearTime / reps << ","
                 << "\"kmpAverage\":" << (double)kmpTime / reps << ","
                 << "\"boyerMooreAverage\":" << (double)boyerMooreTime / reps
                 << "}";
        }

        cout << "]}" << endl;

        return 0;
    }

    // API mode: search documents for the Node.js backend
    if (argc >= 3 && string(argv[1]) == "--search")
    {
        string keyword = toLowerCase(argv[2]);

        cout << "[";
        bool firstResult = true;

        for (const string &filename : filenames)
        {
            ifstream file(filename);

            if (!file.is_open())
                continue;

            string line;
            bool documentMatches = false;

            while (getline(file, line))
            {
                if (boyerMooreSearch(
                        toLowerCase(line), keyword))
                {
                    documentMatches = true;
                    break;
                }
            }

            file.close();

            if (documentMatches)
            {
                if (!firstResult)
                    cout << ",";

                cout << "\"" << jsonEscape(filename) << "\"";
                firstResult = false;
            }
        }

        cout << "]" << endl;

        return 0;
    }

    // Interactive search mode
    string keyword;

    cout << "Enter keyword to search: ";
    cin >> keyword;

    keyword = toLowerCase(keyword);

    bool foundAny = false;

    for (const string &filename : filenames)
    {
        ifstream file(filename);

        if (!file.is_open())
        {
            cout << "Could not open: "
                 << filename << endl;
            continue;
        }

        string line;
        bool foundInFile = false;

        while (getline(file, line))
        {
            if (boyerMooreSearch(
                    toLowerCase(line), keyword))
            {
                foundInFile = true;
                break;
            }
        }

        file.close();

        if (foundInFile)
        {
            cout << "Found in: "
                 << filename << endl;

            foundAny = true;
        }
    }

    if (!foundAny)
        cout << "Keyword not found in any document." << endl;

    // Algorithm performance comparison
    cout << "\n--- Algorithm Performance Comparison ---\n";

    string benchmarkText =
        "merge sort is a sorting algorithm";

    long long naiveTime =
        benchmarkSearch(1, benchmarkText, keyword);

    long long kmpTime =
        benchmarkSearch(2, benchmarkText, keyword);

    long long boyerMooreTime =
        benchmarkSearch(3, benchmarkText, keyword);

    cout << "Naive Search: " << naiveTime
         << " microseconds\n";

    cout << "KMP Search: " << kmpTime
         << " microseconds\n";

    cout << "Boyer-Moore Search: " << boyerMooreTime
         << " microseconds\n";

    // Document similarity analysis
    cout << "\n--- Document Similarity Analysis ---\n";

    double maxSimilarity = -1.0;
    string mostSimilarDoc1 = "";
    string mostSimilarDoc2 = "";

    for (int i = 0; i < (int)filenames.size(); i++)
    {
        for (int j = i + 1; j < (int)filenames.size(); j++)
        {
            string textA = readDocument(filenames[i]);
            string textB = readDocument(filenames[j]);

            set<string> wordsA = getUniqueWords(textA);
            set<string> wordsB = getUniqueWords(textB);

            double similarity =
                calculateJaccardSimilarity(wordsA, wordsB);

            if (similarity > maxSimilarity)
            {
                maxSimilarity = similarity;
                mostSimilarDoc1 = filenames[i];
                mostSimilarDoc2 = filenames[j];
            }

            cout << filenames[i] << " vs "
                 << filenames[j] << " : "
                 << similarity << "% similarity\n";
        }
    }

    cout << "\n--- Most Similar Documents ---\n";
    cout << "Document 1: " << mostSimilarDoc1 << endl;
    cout << "Document 2: " << mostSimilarDoc2 << endl;
    cout << "Similarity: " << maxSimilarity << "%\n";

    return 0;
}
