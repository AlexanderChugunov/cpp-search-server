#include <iostream>
#include <set>
#include <vector>
#include <map>
#include <string>
#include"search_server.h"
#include"request_queue.h"
#include"read_input_functions.h"
#include"paginator.h"

using namespace std;

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
ostream& operator<<(ostream& out, const std::set<Element>& container) {
    out << "{";
    Print(out, container);
    out << "}";
    return out;
}
template <typename Element>
ostream& operator<<(ostream& out, const std::vector<Element>& container) {
    out << "{";
    Print(out, container);
    out << "}";
    return out;
}


template <typename Key, typename Value>
ostream& operator<<(ostream& out, const std::map<Key, Value>& container) {
    out << "{";
    Print(out, container);
    out << "}";
    return out;
}
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint) {

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

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
    const std::string& hint) {
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
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const std::string content = std::string("cat in the city");
    const std::vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments(std::string("in"));
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server(std::string("in the"));

        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}
void TestMinusWords() {
    const int doc_id = 42;
    const std::string content = std::string("cat in the city");
    const std::vector<int> ratings = { 1, 2, 3 };
    const int doc_id_2 = 41;
    const std::string content_2 = std::string("cat city");
    const std::vector<int> ratings_2 = { 1, 2 };
    SearchServer server;
    {
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
        ASSERT_EQUAL(server.FindTopDocuments(std::string("cat -in")).size(), 1); // int int
    }
}
void TestMatch() {
    SearchServer server;
    const int doc_id_3 = 40;
    const std::string content_3 = "cat city like milk"s;
    const std::vector<int> ratings_3 = { 1, 2 };
    server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
    std::tuple<std::vector<std::string>, DocumentStatus> test = { {"cat"s, "like"s}, DocumentStatus::ACTUAL };
    std::tuple<std::vector<std::string>, DocumentStatus> test1 = server.MatchDocument("cat like", 40);
    std::tuple<std::vector<std::string>, DocumentStatus> test2 = server.MatchDocument("cat -milk", 40);
    std::tuple<std::vector<std::string>, DocumentStatus> test3 = { {} , DocumentStatus::ACTUAL };
    std::vector<std::string>bad_copy = get<0>(test);
    std::vector<std::string>bad_copy1 = get<0>(test1);
    std::vector<std::string>bad_copy2 = get<0>(test2);
    std::vector<std::string>bad_copy3 = get<0>(test3);
    ASSERT_EQUAL(bad_copy, bad_copy1); //tuple((1,2,3,..),status)
    ASSERT_EQUAL(bad_copy2, bad_copy3);//tuple((1,2,3,..),status)
}
void TestRelev() {
    SearchServer search_server("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
    std::vector<double> test1 = { 0.866434,0.173287,0.173287 };
    std::vector<Document> copy = search_server.FindTopDocuments("пушистый ухоженный кот"s);
    int i = 0;
    bool flag = true;
    for (auto& x : copy) {
        if (std::abs(x.relevance - test1[i]) < std::numeric_limits<double>::epsilon()) {
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
    const std::string content_4 = "cat city like milk"s;
    const std::vector<int> ratings_4 = { 1, 2, 3 };
    server.AddDocument(doc_id_4, content_4, DocumentStatus::ACTUAL, ratings_4);
    std::vector<Document> copy = server.FindTopDocuments("cat"s);
    int sum = 0;
    for (auto& x : copy) {
        sum += x.rating;
    }
    ASSERT_EQUAL(sum / copy.size(), 2); // int int
}
void TestPredicat() {
    SearchServer server;
    const int doc_id_5 = 38;
    const std::string content_5 = "cat city like milk"s;
    const std::vector<int> ratings_5 = { 1, 2, 3 };
    server.AddDocument(doc_id_5, content_5, DocumentStatus::ACTUAL, ratings_5);
    server.AddDocument(3, "trash"s, DocumentStatus::BANNED, { 9 });
    auto copy = server.FindTopDocuments("milk"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
    ASSERT_EQUAL(copy.size(), 1); //int int
}
void TestStatus() {
    SearchServer search_server("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
    std::vector<Document> copy = search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
    ASSERT_EQUAL(copy.size(), 1); //int int
}
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
    setlocale(LC_ALL, "russian");
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
    //SearchServer search_server("and in at"s);
    //RequestQueue request_queue(search_server);
    //
    //search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    //search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    //search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
    //search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    //search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
    //
    //// 1439 запросов с нулевым результатом
    //for (int i = 0; i < 1439; ++i) {
    //    request_queue.AddFindRequest("empty request"s);
    //}
    //// все еще 1439 запросов с нулевым результатом
    //request_queue.AddFindRequest("curly dog"s);
    //// новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    //request_queue.AddFindRequest("big collar"s);
    //// первый запрос удален, 1437 запросов с нулевым результатом
    //request_queue.AddFindRequest("sparrow"s);
    //cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
    SearchServer search_server("and with"s);
    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
    const auto search_results = search_server.FindTopDocuments("curly dog"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);
    // Выводим найденные документы по страницам
    for (auto page = pages.begin(); page != pages.end(); ++page) {
        cout << *page << endl;
        cout << "Page break"s << endl;
    }
}
