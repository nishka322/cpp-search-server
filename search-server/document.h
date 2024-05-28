#pragma once
#include <iostream>

using namespace std::string_literals;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

// объявление структуры документов с 3 переменными: id, relevance, rating
struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
            : id(id)
            , relevance(relevance)
            , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};


std::ostream& operator<<(std::ostream& out, const Document& document) {
    out << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s;
    return out;
}

// объявление перечисленных типов
enum class DocumentStatus {
    ACTUAL,        // актуальный
    IRRELEVANT,    // устаревший
    BANNED,        // отклонённый
    REMOVED        // удалённый
};