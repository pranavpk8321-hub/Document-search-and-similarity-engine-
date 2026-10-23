
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <sstream>
#include <set>
using namespace std;

string toLowerCase(string text)
{
    transform(text.begin(), text.end(), text.begin(),
              [](unsigned char c)
              {
                  return static_cast<char>(tolower(c));
              });
    return text;
}

bool linearSearch(string text, string pattern)
{
    int n = text.length();
    int m = pattern.length();

    if (m == 0)
    {
        return true;
    }

    if (m > n)
    {
        return false;
    }

    for (int i = 0; i <= n - m; i++)
    {
        int j = 0;

        while (j < m && text[i + j] == pattern[j])
        {
            j++;
        }

        if (j == m)
        {
            return true;
        }
    }

    return false;
}

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
            {
                len = lps[len - 1];
            }
            else
            {
                lps[i] = 0;
                i++;
            }
        }
    }

    return lps;
}

bool kmpSearch(string text, string pattern)
{
    int n = text.length();
    int m = pattern.length();

    if (m == 0)
    {
        return true;
    }

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
        {
            return true;
        }
        else if (i < n && text[i] != pattern[j])
        {
            if (j != 0)
            {
                j = lps[j - 1];
            }
            else
            {
                i++;
            }
        }
    }

    return false;
}

int badCharShift(string pattern, char mismatch, int j)
{
    int last = -1;

    for (int i = 0; i < j; i++)
    {
        if (pattern[i] == mismatch)
        {
            last = i;
        }
    }

    return max(1, j - last);
}

bool boyerMooreSearch(string text, string pattern)
{
    int n = text.length();
    int m = pattern.length();

    if (m == 0)
    {
        return true;
    }

    if (m > n)
    {
        return false;
    }

    int s = 0;

    while (s <= n - m)
    {
        int j = m - 1;

        while (j >= 0 && pattern[j] == text[s + j])
        {
            j--;
        }

        if (j < 0)
        {
            return true;
        }

        s += badCharShift(pattern, text[s + j], j);
    }

    return false;
}

long long benchmarkSearch(int algorithm, string text, string pattern)
{
    const int repetitions = 10000;
    bool result = false;

    auto start = chrono::high_resolution_clock::now();

    for (int i = 0; i < repetitions; i++)
    {
        if (algorithm == 1)
        {
            result = linearSearch(text, pattern);
        }
        else if (algorithm == 2)
        {
            result = kmpSearch(text, pattern);
        }
        else if (algorithm == 3)
        {
            result = boyerMooreSearch(text, pattern);
        }
    }

    auto end = chrono::high_resolution_clock::now();

    // Keep the result observable so the searches aren't treated as unused.
    (void)result;

    return chrono::duration_cast<chrono::microseconds>(
               end - start)
        .count();
}

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
            {
                cleanedWord += c;
            }
        }

        if (!cleanedWord.empty())
        {
            words.insert(cleanedWord);
        }
    }

    return words;
}

double calculateJaccardSimilarity(set<string> wordsA, set<string> wordsB)
{
    int intersection = 0;

    for (const string &word : wordsA)
    {
        if (wordsB.find(word) != wordsB.end())
        {
            intersection++;
        }
    }

    set<string> unionWords = wordsA;
    unionWords.insert(wordsB.begin(), wordsB.end());

    if (unionWords.empty())
    {
        return 0.0;
    }

    double similarity = (double)intersection / unionWords.size();

    return similarity * 100.0;
}

string readDocument(string filename)
{
    ifstream file(filename);
    string content;
    string line;

    if (!file.is_open())
    {
        return "";
    }

    while (getline(file, line))
    {
        content += line + " ";
    }

    file.close();
    return content;
}

int main(int argc, char *argv[])
{
    vector<string> filenames = {
        "documents/sample1.txt",
        "documents/sample2.txt",
        "documents/sample3.txt"};

    // API mode: receive the search keyword from Node.js.
    if (argc >= 3 && string(argv[1]) == "--search")
    {
        string keyword = toLowerCase(argv[2]);
        bool foundAny = false;

        cout << "[";

        bool firstResult = true;

        for (const string &filename : filenames)
        {
            ifstream file(filename);

            if (!file.is_open())
            {
                continue;
            }

            string line;
            bool documentMatches = false;

            while (getline(file, line))
            {
                if (boyerMooreSearch(toLowerCase(line), keyword))
                {
                    documentMatches = true;
                    break;
                }
            }

            if (documentMatches)
            {
                if (!firstResult)
                {
                    cout << ",";
                }

                cout << "\"" << filename << "\"";
                firstResult = false;
                foundAny = true;
            }
        }

        cout << "]" << endl;
        return 0;
    }

    string keyword;

    cout << "Enter keyword to search: ";
    cin >> keyword;

    keyword = toLowerCase(keyword);

    bool foundAny = false;

    for (int i = 0; i < filenames.size(); i++)
    {
        ifstream file(filenames[i]);

        if (!file.is_open())
        {
            cout << "Could not open: " << filenames[i] << endl;
            continue;
        }

        string line;
        bool foundInFile = false;

        while (getline(file, line))
        {
            line = toLowerCase(line);

            if (boyerMooreSearch(line, keyword))
            {
                foundInFile = true;
                break;
            }
        }

        file.close();

        if (foundInFile)
        {
            cout << "Found in: " << filenames[i] << endl;
            foundAny = true;
        }
    }

    if (!foundAny)
    {
        cout << "Keyword not found in any document." << endl;
    }

    cout << "\n--- Algorithm Performance Comparison ---\n";

    long long naiveTime = benchmarkSearch(1,
                                          "merge sort is a sorting algorithm", keyword);

    long long kmpTime = benchmarkSearch(2,
                                        "merge sort is a sorting algorithm", keyword);

    long long boyerMooreTime = benchmarkSearch(3,
                                               "merge sort is a sorting algorithm", keyword);

    cout << "Naive Search: " << naiveTime << " microseconds\n";
    cout << "KMP Search: " << kmpTime << " microseconds\n";
    cout << "Boyer-Moore Search: " << boyerMooreTime << " microseconds\n";

    cout << "\n--- Document Similarity Analysis ---\n";

    double maxSimilarity = -1.0;
    string mostSimilarDoc1 = "";
    string mostSimilarDoc2 = "";

    for (int i = 0; i < filenames.size(); i++)
    {
        for (int j = i + 1; j < filenames.size(); j++)
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

            cout << filenames[i] << " vs " << filenames[j]
                 << " : " << similarity << "% similarity\n";
        }
    }

    cout << "\n--- Most Similar Documents ---\n";
    cout << "Document 1: " << mostSimilarDoc1 << endl;
    cout << "Document 2: " << mostSimilarDoc2 << endl;
    cout << "Similarity: " << maxSimilarity << "%\n";

    return 0;
}
