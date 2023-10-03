#pragma once
#include <string>
#include <vector>
#include <set>
#include <deque>
#include <map>
#include <tuple>
#include <stdexcept>
#include <algorithm>
#include <execution>
#include"document.h"
#include"string_processing.h"
#include"concurrent_map.h"
const int MAX_RESULT_DOCUMENT_COUNT = 5;
constexpr double COMPARISON_ERROR = 1e-6;
class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
    {

        if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            throw std::invalid_argument(std::string("Some of stop words are invalid"));
        }
    }
    explicit SearchServer(std::string const& str)
        : SearchServer(std::string_view(str))

    {
    }
    explicit SearchServer(std::string_view stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor
        // from string container
    {
    }
    explicit SearchServer()
        : SearchServer(std::string(""))

    {
    }

    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;
    template <typename DocumentPredicate, typename exe_policy>
    std::vector<Document> FindTopDocuments(exe_policy policy, const std::string_view raw_query, DocumentPredicate document_predicate) const;
    template <typename exe_policy>
    std::vector<Document> FindTopDocuments(exe_policy policy, const std::string_view raw_query, DocumentStatus status) const;
    template <typename exe_policy>
    std::vector<Document> FindTopDocuments(exe_policy policy, const std::string_view raw_query) const;

    std::set<int>::iterator begin();
    std::set<int>::const_iterator begin() const;
    std::set<int>::iterator end();
    std::set<int>::const_iterator end() const;
    const std::set<std::string_view>& GetWordFrequencies(int document_id) const;
    int GetDocumentCount() const;
    void RemoveDocument(int document_id);
    void RemoveDocument(std::execution::sequenced_policy, int document_id);
    void RemoveDocument(std::execution::parallel_policy, int document_id);

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument
    (std::execution::sequenced_policy, std::string_view raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument
    (std::execution::parallel_policy, std::string_view raw_query, int document_id) const;

    std::map<int, std::set<std::string_view>> document_to_word_freqs_;
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    //std::map<int, std::set<std::string>> document_to_word_freqs_; вынес в public тк в функции remuve_duolicate не видит
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;
    std::deque<std::string> storage;


    bool IsStopWord(std::string_view word) const;

    static bool IsValidWord(std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;


    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(std::string_view text, bool flag) const;
    // Existence required
    double ComputeWordInverseDocumentFreq(std::string_view word) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy&, const Query& query, DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&, const Query& query, DocumentPredicate document_predicate) const;
};
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const {
    return SearchServer::FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename DocumentPredicate, typename exe_policy>
std::vector<Document> SearchServer::FindTopDocuments(exe_policy policy, const std::string_view raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query, true);

    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < COMPARISON_ERROR) {
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

template <class exe_policy>
std::vector<Document> SearchServer::FindTopDocuments(exe_policy policy, const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

template <class exe_policy>
std::vector<Document> SearchServer::FindTopDocuments(exe_policy policy, const std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    return SearchServer::FindAllDocuments(std::execution::seq, query, document_predicate);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy&, const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy&, const Query& query, DocumentPredicate document_predicate) const {

    ConcurrentMap<int, int> minus_ids(16);
    for_each(
        std::execution::par,
        query.minus_words.begin(),
        query.minus_words.end(),
        [this, &minus_ids](const std::string_view word) {
            if (word_to_document_freqs_.count(std::string(word))) {
                for (const auto& document_freqs : word_to_document_freqs_.at(std::string(word))) {
                    minus_ids[document_freqs.first];
                }
            }
        }
    );

    auto minus = minus_ids.BuildOrdinaryMap();
    ConcurrentMap<int, double> document_to_relevance(10000);
    int one_part = 16;
    const auto part_length = query.plus_words.size() / one_part;
    auto part_begin = query.plus_words.begin();
    auto part_end = next(part_begin, part_length);

    std::vector<std::future<void>> futures;
    for (int i = 0; i < one_part;  ++i,
        part_begin = part_end, part_end = (i == one_part - 1 ? query.plus_words.end() : next(part_begin, part_length))
        ) {
        futures.push_back(std::async([this, part_begin, part_end, &document_predicate, &document_to_relevance, &minus] {
            for_each(std::execution::par,
            part_begin,
            part_end,
            [this, &document_predicate, &document_to_relevance, &minus](std::string_view word) {
                    if (word_to_document_freqs_.count(std::string(word))) {
                        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(std::string(word))) {
                            const auto& document_data = documents_.at(document_id);
                            if (document_predicate(document_id, document_data.status, document_data.rating) && (minus.count(document_id) == 0)) {
                                document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                            }
                        }
                    }
                });
            }));
    }

    for (auto& stage : futures) {
        stage.get();
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}
