#include "remove_duplicates.h"

//void RemoveDuplicates(SearchServer& search_server)
//{
//    std::set<int> duplicates;
//
//    for (auto document_id = search_server.begin(); document_id != search_server.end(); ++document_id) {
//        for (auto next_document_id = std::next(document_id); next_document_id != search_server.end();++next_document_id){
//            auto duplicate_id = search_server.GetWordFrequencies(*document_id);                      //дублирование карт + исаользование доп функции GetWordFrequencies
//            auto duplicate_next_id = search_server.GetWordFrequencies(*next_document_id);
//            if  ((duplicate_id.size() != duplicate_next_id.size()) &&                               // проверка соответствия размеров
//                (std::equal(duplicate_id.begin(), duplicate_id.end(),duplicate_next_id.begin())))   // проверка равенства
//                { 
//                duplicates.insert(*next_document_id);
//            }
//
//        }
//    }
//
//    for (int id : duplicates)
//    {
//        std::cout << "Found duplicate document id " << id << '\n';
//        search_server.RemoveDocument(id);
//    }
//}
bool CompareFreqMaps(const std::map<std::string, double>& map1,
    const std::map<std::string, double>& map2)
{
    if (map1.size() != map2.size()) return false;

    std::map<std::string, double>::const_iterator pos1 = map1.begin();
    std::map<std::string, double>::const_iterator pos2 = map2.begin();

    while (pos1 != map1.end())
    {
        if ((*pos1).first != (*pos2).first) return false;
        ++pos1; ++pos2;
    }

    return true;
}
void RemoveDuplicates(SearchServer& search_server)
{
    std::set<int> duplicates;
    std::set<std::set<std::string>> document_word;
    for (auto document_id = search_server.begin(); document_id != search_server.end(); ++document_id) {
            auto doc_id = search_server.GetWordFrequencies(*document_id);                //дублирование карт + исаользование доп функции GetWordFrequencies
            std::set<std::string> dupli = search_server.document_to_word_freqs_.at(*document_id);
            if (document_word.count(dupli)) {
                duplicates.insert(*document_id);
            }
            else {
                document_word.insert(dupli);
            }

    }

    for (int id : duplicates)
    {
        std::cout << "Found duplicate document id " << id << '\n';
        search_server.RemoveDocument(id);
    }
}
