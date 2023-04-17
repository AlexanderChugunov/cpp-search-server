
#include <iostream>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

/* Подставьте вашу реализацию класса SearchServer сюда */
const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
        const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }
    bool predicat(int& id, DocumentStatus status, double& rating) {
        bool flag = true;
        return flag;
    }
    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus Status) const {
        return FindTopDocuments(raw_query, [Status](int document_id, DocumentStatus status, int rating) { return status == Status; });
    }
    template<typename Predicat>
    vector<Document> FindTopDocuments(const string& raw_query,
        Predicat predicat) const {

        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, predicat);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
        int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }
    template<typename Predicat>
    vector<Document> FindAllDocuments(const Query& query, Predicat predicat) const {
        map<int, double> document_to_relevance;

        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                //documents_.at(document_id).status == status
                if (predicat(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

template <typename First, typename Second>
ostream& operator<<(ostream& out, const pair<First, Second>& p) {
    return out << p.first << ": "s << p.second;
}

template <typename Container>
void Print(ostream& out, const Container& container) {
    bool is_first = true;
    for (const auto& element : container) {
        if (!is_first) {
            out << ", "s;
        }
        is_first = false;
        out << element;
    }
}


template <typename Element>
ostream& operator<<(ostream& out, const set<Element>& container) {
    out << "{";
    Print(out, container);
    out << "}";
    return out;
}
template <typename Element>
ostream& operator<<(ostream& out, const vector<Element>& container) {
    out << "{";
    Print(out, container);
    out << "}";
    return out;
}


template <typename Key, typename Value>
ostream& operator<<(ostream& out, const map<Key, Value>& container) {
    out << "{";
    Print(out, container);
    out << "}";
    return out;
}


template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {

    if (u != t) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }

}



#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))
// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}
void TestMinusWords() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    const int doc_id_2 = 41;
    const string content_2 = "cat city"s;
    const vector<int> ratings_2 = { 1, 2 };
    SearchServer server;
    {
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        ASSERT_EQUAL(server.FindTopDocuments("cat -in"s).size(), 1); // int int
    }
}
void TestMatch() {
    SearchServer server;
    const int doc_id_3 = 40;
    const string content_3 = "cat city like milk"s;
    const vector<int> ratings_3 = { 1, 2 };
    server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
    tuple<vector<string>, DocumentStatus> test = { {"cat"s, "like"s}, DocumentStatus::ACTUAL };
    tuple<vector<string>, DocumentStatus> test1 = server.MatchDocument("cat like", 40);
    tuple<vector<string>, DocumentStatus> test2 = server.MatchDocument("cat -milk", 40);
    tuple<vector<string>, DocumentStatus> test3 = { {} , DocumentStatus::ACTUAL };
    vector<string>bad_copy = get<0>(test);
    vector<string>bad_copy1 = get<0>(test1);
    vector<string>bad_copy2 = get<0>(test2);
    vector<string>bad_copy3 = get<0>(test3);
    ASSERT_EQUAL(bad_copy, bad_copy1); //tuple((1,2,3,..),status)
    ASSERT_EQUAL(bad_copy2, bad_copy3);//tuple((1,2,3,..),status)
}
void TestRelev() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
    vector<double> test1 = { 0.866434,0.173287,0.173287 };
    vector<Document> copy = search_server.FindTopDocuments("пушистый ухоженный кот"s);
    int i = 0;
    bool flag = true;
    for (auto& x : copy) {
        if (abs(x.relevance - test1[i]) < std::numeric_limits<double>::epsilon()) {
            flag = false;
            break;
        }
        ++i;
    }
    ASSERT(flag);
}
void TestRaiting() {
    SearchServer server;
    const int doc_id_4 = 39;
    const string content_4 = "cat city like milk"s;
    const vector<int> ratings_4 = { 1, 2, 3 };
    server.AddDocument(doc_id_4, content_4, DocumentStatus::ACTUAL, ratings_4);
    vector<Document> copy = server.FindTopDocuments("cat"s);
    int sum = 0;
    for (auto& x : copy) {
        sum += x.rating;
    }
    ASSERT_EQUAL(sum / copy.size(), 2); // int int
}
void TestPredicat() {
    SearchServer server;
    const int doc_id_5 = 38;
    const string content_5 = "cat city like milk"s;
    const vector<int> ratings_5 = { 1, 2, 3 };
    server.AddDocument(doc_id_5, content_5, DocumentStatus::ACTUAL, ratings_5);
    server.AddDocument(3, "trash"s, DocumentStatus::BANNED, { 9 });
    auto copy = server.FindTopDocuments("milk"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
    ASSERT_EQUAL(copy.size(), 1); //int int
}
void TestStatus() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
    vector<Document> copy = search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
    ASSERT_EQUAL(copy.size(), 1); //int int
}
/*
Разместите код остальных тестов здесь
*/

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    TestExcludeStopWordsFromAddedDocumentContent();
    TestMinusWords();
    TestMatch();
    TestRelev();
    TestRaiting();
    TestPredicat();
    TestStatus();

    // Не забудьте вызывать остальные тесты здесь
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}


