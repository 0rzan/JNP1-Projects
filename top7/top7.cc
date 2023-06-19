#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <map>
#include <regex>
using namespace std;

namespace {
    using vote_pair = pair<unsigned int, uint32_t>; // para (liczba głosów, id)

    // komparator vote_pair, sortuje najpierw po liczbie głosów rosnąco, potem po id malejąco
    struct vote_pair_compare {
        bool operator()(const vote_pair &a, const vote_pair &b) const {
            return a.first == b.first ? a.second < b.second : a.first > b.first; 
        }
    };

    static const regex pattern_new_record("\\s*NEW\\s+([0]*[1-9][0-9]{0,7})\\s*");
    static const regex pattern_top("\\s*TOP\\s*");
    static const regex pattern_empty_line("\\s*");
    static const regex pattern_votes("\\s*([0]*[1-9][0-9]{0,7}\\s+)*[0]*[1-9][0-9]{0,7}\\s*");

    static constexpr uint32_t MAX_ID = 99999999;
    static constexpr size_t MAX_TOP = 7; // maksymalna liczba utworów wypisywanych w podsumowaniu

    uint32_t current_max_id = 0; // największy dopuszczalny numer w obecnym notowaniu
    unsigned int current_line = 0;

    unordered_map<uint32_t, unsigned int> votes; // liczba głosów oddanych na utwór w obecnym notowaniu
    unordered_map<uint32_t, unsigned int> total_points; // liczba punktów w łącznym rankingu
    set<vote_pair, vote_pair_compare> top_current; // elementy są parami (liczba głosów, id)
    set<vote_pair, vote_pair_compare> top_total; // jak wyżej
    unordered_map<uint32_t, uint8_t> prev_rank; // miejce w poprzednim notowaniu
    unordered_map<uint32_t, uint8_t> prev_rank_total; // miejsce w poprzednim wywołaniu top
    unordered_set<uint32_t> illegal_ids; // utwory które wypadły z listy przebojów

    // Funkcja aktualizuje zbiór top 7 utworów, jeśli utwór o id x.second i liczbie głosów x.first dostał dodatkowo delta punktów
    // Zastosowania:
    //      - przy globalnym top7 delta jest liczbą punktów otrzymaną pod koniec notowania
    //      - przy top7 w obecnym notowaniu delta jest równa jeden (symuluje otrzymanie jednego głosu na dany utwór)
    void update(vote_pair x, uint8_t delta, set<vote_pair, vote_pair_compare>& s) {
        s.erase(x);
        s.insert(make_pair(x.first + delta, x.second));

        // Zmniejszamy rozmiar seta, żeby otrzymać lepszą złożoność
        while (s.size() > MAX_TOP)
            s.erase(prev(s.end()));
    }

    // Wypisuje top 7 utworów w łącznym rankingu (jeśli parametry to top_total i prev_rank_total)
    // lub w obecnym notowaniu (jeśli parametry to top_current i prev_rank)
    void summarize(const set<vote_pair, vote_pair_compare> &top, unordered_map<uint32_t, uint8_t> &rank) {
        uint8_t current_rank = 1; 
        for (const auto &[points, id] : top) {
            cout << id << " ";

            if (!rank.contains(id))
                cout << "-" << endl;
            else {
                int16_t delta = static_cast<int16_t>(rank[id]) - static_cast<int16_t>(current_rank);
                cout << delta << endl;
            }

            current_rank++;
        }
    }

    // Oddaje głos na utwór o numerze id
    void add_vote(int id) {
        update(make_pair(votes[id], id), 1, top_current);
        votes[id]++;
    }

    void top() {
        summarize(top_total, prev_rank_total);
        prev_rank_total.clear();

        uint8_t current_rank = 1;
        for (const auto &[rank, id] : top_total)
            prev_rank_total[id] = current_rank++;
    }

    void new_record(int new_max_id) {
        summarize(top_current, prev_rank);

        for (const auto &[id, rank] : prev_rank) {
            if (!top_current.contains(make_pair(votes[id], id)))
                illegal_ids.insert(id);
        }

        prev_rank.clear();
        
        // poniższy for aktualizuje globalny ranking
        uint8_t current_rank = 1;
        for (const auto &[n_votes, id] : top_current) {
            uint8_t delta = MAX_TOP - current_rank + 1;
            update(make_pair(total_points[id], id), delta, top_total);
            total_points[id] += delta;
            prev_rank[id] = current_rank++;
        }

        top_current.clear();
        votes.clear();
        current_max_id = new_max_id;
    }

    // Odzyskuje liczby ze stringa i zwraca je w wektorze
    vector<uint32_t> extract_numbers(const string& line) {
        stringstream sstream;
        string word;
        uint32_t number;
        vector<uint32_t> res;

        sstream << line;
        while (!sstream.eof()) {
            sstream >> word;
            if (stringstream(word) >> number)
                res.push_back(number);

            word.clear();
        }
        
        return res;
    }

    void call_error(const string& line) {
        cerr << "Error in line " << current_line << ": " << line << endl; 
    }
}

int main() {
    string line;
    while (getline(cin, line)) {
        current_line++;

        if (regex_match(line, pattern_votes)) {
            vector<uint32_t> numbers = extract_numbers(line);

            // Sprawdzamy, czy głosy się nie powtarzają
            sort(numbers.begin(), numbers.end());
            if (adjacent_find(numbers.begin(), numbers.end()) != numbers.end()) {
                call_error(line);
                continue;
            }

            // Sprawdzamy, czy głos nie wypadł z notowania
            bool valid = true;
            for (uint32_t x : numbers) {
                if (x > current_max_id || illegal_ids.contains(x)) {
                    call_error(line);
                    valid = false;
                    break;
                }
            }

            if (!valid) 
                continue;

            for (uint32_t x : numbers)
                add_vote(x);
        }
        else if (regex_match(line, pattern_new_record)) {;
            uint32_t new_max_id = extract_numbers(line)[0];
            if (new_max_id > MAX_ID || new_max_id < current_max_id)
                call_error(line);
            else
                new_record(new_max_id);
        }
        else if (regex_match(line, pattern_top))
            top();
        else if (!regex_match(line, pattern_empty_line))
            call_error(line);
    }
 
    return 0;
}
