// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь: 271

// Закомитьте изменения и отправьте их в свой репозиторий.

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}

struct Document {
    int id;
    double relevance;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        ++document_count_; //добавляем документ
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double TFup = 1.0/words.size();
//Можно пройтись циклом по всему документу, увеличивая TF каждого встречаемого слова...
        for (const string& word : words){
            word_to_document_freqs_[word][document_id] += TFup;
        }
    }
    vector<Document> FindTopDocuments(const string& raw_query) const {

        const Query query_words = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:

    ///которая сопоставляет каждому слову словарь «документ → TF».
    map<string, map<int, double>> word_to_document_freqs_;
    int document_count_ = 0; //...в поисковой системе должно храниться количество документов.

    struct Query
    {
        set<string> plus_words;
        set<string> minus_words;
    };

    set<string> stop_words_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const string& text) const{
        Query query;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            if (word[0] == '-'){
                query.minus_words.insert(word.substr(1));
            }
            else{
                query.plus_words.insert(word);
            }
        }
        return query;
    }

    double CalculateFreq(const string& word) const {
        return log(document_count_ * 1.0 / word_to_document_freqs_.at(word).size());
    }

       vector<Document> FindAllDocuments(const Query& query_words) const{
        map<int, double> document_to_relevance;
        vector<Document> matched_documents;
//для плюс слов
        for (const string& word : query_words.plus_words) {
            if (word_to_document_freqs_.count(word) == 0){
                continue;
            }
            const double inverse_freq = CalculateFreq(word);
            for (const auto& [document_id, t_freq] : word_to_document_freqs_.at(word) ) {
                document_to_relevance[document_id] += t_freq * inverse_freq;
            }
        }
//для минус слов
           for (const string& word : query_words.minus_words) {
               if (word_to_document_freqs_.count(word) == 0) {
                   continue;
               }
               for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
                   document_to_relevance.erase(document_id);
               }
           }

           for (const auto& [document_id, relevance] : document_to_relevance) {
               matched_documents.push_back({document_id, relevance});
           }
           return matched_documents;
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());
    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }
    return search_server;
}


int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}