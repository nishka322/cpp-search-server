#include "search_server.h"

// функция добавления слов поискового запроса (без стоп-слов) в documents_
//Метод AddDocument выбрасывет исключение invalid_argument в следующих ситуациях:
void SearchServer::AddDocument(int document_id, const string& document, DocumentStatus status,
                 const vector<int>& ratings) {
    if ((document_id < 0) || documents_.count(document_id)) {//Попытка добавить документ с отрицательным id;
        throw invalid_argument("Document id less then zero");
    }
    if (documents_.count(document_id)){//Попытка добавить документ c id ранее добавленного документа;
        throw invalid_argument("repeat document id");
    }
    const vector<string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids.push_back(document_id);
    //return 0;
}

vector<Document>  SearchServer::FindTopDocuments(const string& raw_query, DocumentStatus status) const{ //Если тут задать статус по умолчанию, то FindTopDocuments(const string& raw_query) будет не нужен
    if (IsValidWord(raw_query) == false){
        throw invalid_argument("Invalid word in FindTopDocument function");
    }
    auto predict = [status](int document_id, DocumentStatus doc_status, int rating) {
        return doc_status == status;
    };
    return FindTopDocuments(raw_query, predict);
}

//Метод должен возвращать количество документов в поисковой системе.
int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

//В первом элементе кортежа верните все плюс-слова запроса, содержащиеся в документе.
// Слова не должны дублироваться. Отсортированы по возрастанию.
// Если нет пересечений по плюс-словам или есть минус-слово, вектор слов вернуть пустым.
tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(const string& raw_query,
                                                    int document_id)
// Если документ не соответствует запросу(нет пересечений по плюс - словам
// или есть минус - слово), вектор слов нужно вернуть пустым.
const {
    if (IsValidWord(raw_query) == false){
        throw invalid_argument("Invalid word in MatchDocument function");
    }
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
    auto result = tuple{matched_words, documents_.at(document_id).status};
    return result;
}

int SearchServer::GetDocumentId(const int index) const {
    return document_ids.at(index);
}

bool SearchServer::IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0;
}

// функция считывания слов поискового запроса и удаления из него стоп-слов (считывание плюс-слов)
vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
    vector<string> words;
    for (const string& word : SplitIntoWords(text)) {
        if (IsValidWord(word) == false) {//Наличие недопустимых символов (с кодами от 0 до 31) в тексте добавляемого документа.
            throw invalid_argument("Invalid word");
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    // Use std::accumulate вместо цикла
    if (ratings.empty()) return 0;
    return accumulate(begin(ratings), end(ratings), 0)
           / static_cast<int>(ratings.size());
}

// обработка минус-слов запроса
SearchServer::QueryWord SearchServer::ParseQueryWord(string text) const {
    QueryWord result;
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (IsValidWord(text) == false) {//Наличие недопустимых символов (с кодами от 0 до 31) в тексте добавляемого документа.
        throw invalid_argument("Invalid word. Words ASCII 0-31.");
    }
    if (text.empty() || text[0] == '-') {//.empty отсекает "кот -", text[0] == '-' отсекает "--кот"
        throw invalid_argument("Invalid minus word in ParseQueryWord"s);
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const string& text) const {
    Query query;
    for (const string& word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            } else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}

// Existence required
// вычисляем IDF - делим количество документов
// где встречается слово на количество всех документов и берём нат.логарифм
double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

bool SearchServer::IsValidWord(const string& word) {//проверка слова на наличие спецсимволов
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}