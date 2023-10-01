#include"search_server.h"
#include <utility>
#include <cmath>
#include <numeric>
#include <deque>
void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status,
    const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument(std::string("Invalid document_id"));
    }
    storage.emplace_back(std::string(document));
    //const auto words = SplitIntoWordsNoStop(document);
    const auto words = SplitIntoWordsNoStop(storage.back());
    const double inv_word_count = 1.0 / words.size();
    for (std::string_view word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id].insert(word);
    }

    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}


int SearchServer::GetDocumentCount() const {
    return documents_.size();
}
std::set<int>::iterator SearchServer::begin()
{
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::begin() const
{
    return document_ids_.begin();
}

std::set<int>::iterator SearchServer::end()
{
    return document_ids_.end();
}

std::set<int>::const_iterator SearchServer::end() const
{
    return document_ids_.end();
}
const std::set<std::string_view>& SearchServer::GetWordFrequencies(int document_id) const {
    if (document_to_word_freqs_.count(document_id) == 0) {
        static const std::set < std::string_view > mySet = {};
        return mySet;
    }
    return document_to_word_freqs_.at(document_id);
}


void SearchServer::RemoveDocument(int document_id)
{
    if (documents_.count(document_id) > 0)
    {
        for (auto word : document_to_word_freqs_.at(document_id)) {
            word_to_document_freqs_[word].erase(document_id);
        }
        documents_.erase(document_id);
        document_to_word_freqs_.erase(document_id);
        document_ids_.erase(document_id);
    }
}
void SearchServer::RemoveDocument(std::execution::sequenced_policy exe_sequence_, int document_id) {
    RemoveDocument(document_id);
}
void SearchServer::RemoveDocument(std::execution::parallel_policy exe_sequence_, int document_id) {
    if (documents_.count(document_id) > 0)
    {
        const auto& copy_doc_twf = document_to_word_freqs_.at(document_id);
        std::vector<std::string_view> vector_pointer_(copy_doc_twf.size());
        std::transform(exe_sequence_, copy_doc_twf.begin(), copy_doc_twf.end(), vector_pointer_.begin(),
            [](const auto& x) {
                return x;
            });
        for_each(exe_sequence_, vector_pointer_.begin(), vector_pointer_.end(),
            [document_id, this](std::string_view word) {
                word_to_document_freqs_.at(word).erase(document_id);
            });
        documents_.erase(document_id);
        document_to_word_freqs_.erase(document_id);
        document_ids_.erase(document_id);
    }
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query, int document_id) const {
    auto query = ParseQuery(raw_query);

    for (std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            return { std::vector<std::string_view>{}, documents_.at(document_id).status };
        }
    }
    std::vector<std::string_view> matched_words;
    for (std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }


    return { matched_words, documents_.at(document_id).status };
}
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument
(std::execution::sequenced_policy exe_sequence_, std::string_view raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument
(std::execution::parallel_policy policy, std::string_view raw_query, int document_id) const {

    const auto query = ParseQuery_Policy(raw_query);

    //проверка на -слова
    if (std::any_of(policy, query.minus_words.begin(), query.minus_words.end(),
        [this, document_id](std::string_view s) {
            if (word_to_document_freqs_.count(s) == 0) {
                return false;
            }
            if (word_to_document_freqs_.at(s).count(document_id)) {
                return true;
            }
            return false;
        }))
    {
        return { std::vector<std::string_view>{}, documents_.at(document_id).status };
    }

    std::vector<std::string_view> matched_words(query.plus_words.size());
    /*std::cout << "aaaa ";*/
    std::copy_if(policy, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(),
        [this, document_id](std::string_view s) {

            if (word_to_document_freqs_.count(s) == 0) {
                return false;
            }
            if (word_to_document_freqs_.at(s).count(document_id)) {
                return true;
            }
            return false;
        });

    std::sort(matched_words.begin(), matched_words.end());
    auto last = std::unique(matched_words.begin(), matched_words.end());
    matched_words.erase(last, matched_words.end());

    if (matched_words[0] == "") {
        matched_words.erase(matched_words.begin());
    }

    return { matched_words, documents_.at(document_id).status };
}


bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(std::string_view word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
    std::vector<std::string_view> words;
    for (std::string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument(std::string("Word ") + std::string(word) + std::string(" is invalid"));
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument(std::string("Query word is empty"));
    }
    std::string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument(std::string("Query word ") + std::string(text) + std::string(" is invalid"));
    }

    return { word, is_minus, IsStopWord(word) };
}


SearchServer::Query SearchServer::ParseQuery(std::string_view text) const {
    Query result;
    for (std::string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    std::sort(result.plus_words.begin(), result.plus_words.end());
    auto last = std::unique(result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(last, result.plus_words.end());

    std::sort(result.minus_words.begin(), result.minus_words.end());
    last = std::unique(result.minus_words.begin(), result.minus_words.end());
    result.minus_words.erase(last, result.minus_words.end());
    return result;
}
SearchServer::Query SearchServer::ParseQuery_Policy(std::string_view text) const {
    Query result;
    for (std::string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    return result;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
//----------no policy
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}
//----------------seq policy
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy, std::string_view raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(raw_query, document_predicate);
}
std::vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy, std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, status);
}
std::vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy, std::string_view raw_query) const {
    return FindTopDocuments(raw_query);
}
//----------------par policy
std::vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy policy,std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy,raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy policy,std::string_view raw_query) const {
    return FindTopDocuments(policy,raw_query, DocumentStatus::ACTUAL);
}
