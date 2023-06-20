#include"document.h"
Document::Document() = default;
Document::Document(int id, double relevance, int rating){
Document::id = id;
Document::relevance = relevance;
Document::rating = rating;
}
