#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server)
{
    std::set<int> duplicates;

    for (auto document_id = search_server.begin(); document_id != search_server.end(); ++document_id) {
        for (auto next_document_id = std::next(document_id); next_document_id != search_server.end(); ++next_document_id) {
            auto doc_id = search_server.GetWordFrequencies(*document_id);                //дублирование карт + исgпользование доп функции GetWordFrequencies
            auto duplicate_next_id = search_server.GetWordFrequencies(*next_document_id);
            if (doc_id == duplicate_next_id) {
                duplicates.insert(*next_document_id);
            }

        }
    }

    for (int id : duplicates)
    {
        std::cout << "Found duplicate document id " << id << '\n';
        search_server.RemoveDocument(id);
    }
}
