#pragma once
#include <algorithm>
#include <cmath>
#include "document.h"
#include <iostream>
#include <map>
#include <numeric>
#include "read_input_functions.h"
#include "string_processing.h"
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
//#include "paginator.h"
//#include "request_queue.h"

using namespace std;

// механизм поиска
class SearchServer {
public:
    // Вместо SetStopWords

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
            : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        //Конструктор класса SearchServer выбрасывать исключение
        // invalid_argument если любое из переданных стоп-слов содержит недопустимые символы
        for(const auto& stop_word: stop_words_){
            if(IsValidWord(stop_word) == false){
                throw invalid_argument("invalid word in class constructor");
            }
        }
    }

    explicit SearchServer(const string& stop_words_text)
            : SearchServer(
            SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }
// функция добавления слов поискового запроса (без стоп-слов) в documents_
//Метод AddDocument выбрасывет исключение invalid_argument в следующих ситуациях:
    void AddDocument(int document_id, const string& document, DocumentStatus status,
                     const vector<int>& ratings);

    // Фильтрация документов должна производиться до отсечения топа из пяти штук.
    // функция вывода 5 наиболее релевантных результатов из всех найденных

    vector<Document>  FindTopDocuments(const string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const;

    template<typename predicate>
    vector<Document> FindTopDocuments(const string& raw_query, predicate predict) const {
        if (IsValidWord(raw_query) == false){
            throw invalid_argument("Invalid word in FindTopDocument function");
        }
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, predict);
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < numeric_limits<double>::epsilon()) { //Избавились от магических чисел через стандартный std::numeric_limits<double>::epsilon()
                     return lhs.rating > rhs.rating;
                 } //else {  else не обязателен, т.к. используется return
                 return lhs.relevance > rhs.relevance;
                 //}
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

//Метод должен возвращать количество документов в поисковой системе.
    int GetDocumentCount() const;

//В первом элементе кортежа верните все плюс-слова запроса, содержащиеся в документе.
// Слова не должны дублироваться. Отсортированы по возрастанию.
// Если нет пересечений по плюс-словам или есть минус-слово, вектор слов вернуть пустым.
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
                                                        int document_id)
    // Если документ не соответствует запросу(нет пересечений по плюс - словам
    // или есть минус - слово), вектор слов нужно вернуть пустым.
    const;

    int GetDocumentId(const int index) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> document_ids; //для хранения айдишников

    bool IsStopWord(const string& word) const;

// функция считывания слов поискового запроса и удаления из него стоп-слов (считывание плюс-слов)
    vector<string> SplitIntoWordsNoStop(const string& text) const;

    static int ComputeAverageRating(const vector<int>& ratings);

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    // обработка минус-слов запроса
    QueryWord ParseQueryWord(string text) const;

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const;

    // Existence required
    // вычисляем IDF - делим количество документов
    // где встречается слово на количество всех документов и берём нат.логарифм
    double ComputeWordInverseDocumentFreq(const string& word) const;

// функция вывода ВСЕХ найденных результатов по релевантности по формуле TF-IDF
    template<typename DocPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocPredicate doc_pred) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_info = documents_.at(document_id);
                if (doc_pred(document_id, document_info.status, document_info.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
        vector<Document> matched_documents;
        for (const auto& [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                    {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }

    static bool IsValidWord(const string& word);
};
