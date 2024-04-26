#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric> // добавлено для std::accumulate
#include <set>
#include <string>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
// функция считывания слов
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}
// функция считывания количества слов
int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}
// функция разбиения на слова и записи в вектор слов words
vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}
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

template <typename StringCollection>
set<string> MakeUniqueNonEmptyStrings(const StringCollection& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}
// объявление перечисленных типов
enum class DocumentStatus {
    ACTUAL,        // актуальный
    IRRELEVANT,    // устаревший
    BANNED,        // отклонённый
    REMOVED        // удалённый
};
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
    // Фильтрация документов должна производиться до отсечения топа из пяти штук.
    // функция вывода 5 наиболее релевантных результатов из всех найденных

    vector<Document>  FindTopDocuments(const string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const{ //Если тут задать статус по умолчанию, то FindTopDocuments(const string& raw_query) будет не нужен
        if (IsValidWord(raw_query) == false){
            throw invalid_argument("Invalid word in FindTopDocument function");
        }
        auto predict = [status](int document_id, DocumentStatus doc_status, int rating) {
            return doc_status == status;
        };
        return FindTopDocuments(raw_query, predict);
    }

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
    int GetDocumentCount() const {
        return documents_.size();
    }
//В первом элементе кортежа верните все плюс-слова запроса, содержащиеся в документе.
// Слова не должны дублироваться. Отсортированы по возрастанию.
// Если нет пересечений по плюс-словам или есть минус-слово, вектор слов вернуть пустым.
 tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
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

    int GetDocumentId(const int index) const {
        return document_ids.at(index);
    }


private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> document_ids; //для хранения айдишников

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

// функция считывания слов поискового запроса и удаления из него стоп-слов (считывание плюс-слов)
    vector<string> SplitIntoWordsNoStop(const string& text) const {
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

    static int ComputeAverageRating(const vector<int>& ratings) {
        // Use std::accumulate вместо цикла
        if (ratings.empty()) return 0;
        return accumulate(begin(ratings), end(ratings), 0)
               / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    // обработка минус-слов запроса
    QueryWord ParseQueryWord(string text) const {
        QueryWord result;
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        if (text.empty() || text[0] == '-') {//.empty отсекает "кот -", text[0] == '-' отсекает "--кот"
            throw invalid_argument("Invalid ParseQueryWord"s);
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
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }
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

    static bool IsValidWord(const string& word) {//проверка слова на наличие спецсимволов
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }
};

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}
int main() {
    SearchServer search_server("and"s);
    search_server.AddDocument(0, "белый кот c модным ошейником"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "ACTUAL:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; })) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    return 0;
}
