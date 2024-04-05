#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <functional>

using namespace std;

// present node
struct Present {
    int tag;
    Present* next_present;
    Present(int t) : tag(t), next_present(nullptr) {}
};

// thank you card node
struct ThankYouCard {
    int guest;
    ThankYouCard(int g) : guest(g) {}
};

class LinkedList {
private:
    Present* head;
    mutex mtx;
    int presents_count;
    int thank_you_cards_count;

public:
    LinkedList() : head(nullptr), presents_count(0), thank_you_cards_count(0) {}

    // add present to appropriate place in linked list
    void add_present(Present* present) {
        lock_guard<mutex> lock(mtx);
        Present* new_present = new Present(present->tag);
        if (!head || head->tag > present->tag) {
            new_present->next_present = head;
            head = new_present;
        } else {
            Present* current = head;
            Present* previous = nullptr;
            while (current && current->tag < present->tag) {
                previous = current;
                current = current->next_present;
            }
            new_present->next_present = current;
            if (previous) {
                previous->next_present = new_present;
            } else {
                head = new_present; // Inserting at the beginning
            }
        }
        presents_count++;
    }

    // simulates writing thank you card
    // presents count is decreased when thank you card is written
    void write_thank_you_card(ThankYouCard* card) {
        lock_guard<mutex> lock(mtx);
        if (!head) return;
        Present* current = head;
        Present* previous = nullptr;
        while (current && current->tag != card->guest) {
            previous = current;
            current = current->next_present;
        }
        if (!current) 
            return;
        if (!previous)
            head = current->next_present;
        else
            previous->next_present = current->next_present;
        delete current; // remove node from linked list
        presents_count--; // decrease presents count (thank you card is written for present)
        thank_you_cards_count++; // increse thank you card count 
    }

    // get number of presents left over
    int getPresentsCount() const {
        return presents_count;
    }

    // get number of thank you cards left over
    int getThankYouCardsCount() const {
        return thank_you_cards_count;
    }
};

void process_presents(LinkedList& linked_list, queue<Present*>& presents_queue, mutex& mtx) {
    while (true) {
        Present* present = nullptr;
        {
            lock_guard<mutex> lock(mtx);
            if (presents_queue.empty()) break;
            present = presents_queue.front();
            presents_queue.pop();
        }
        linked_list.add_present(present);
        delete present;
    }
}

void process_thank_you_cards(LinkedList& linked_list, queue<ThankYouCard*>& cards_queue, mutex& mtx) {
    while (true) {
        ThankYouCard* card = nullptr;
        {
            lock_guard<mutex> lock(mtx);
            if (cards_queue.empty()) break;
            card = cards_queue.front();
            cards_queue.pop();
        }
        linked_list.write_thank_you_card(card);
        delete card;
    }
}

int main() {
    LinkedList linked_list;
    queue<Present*> presents_queue;
    queue<ThankYouCard*> cards_queue;

    // starting program clock
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 1; i <= 50000; ++i) {
        presents_queue.push(new Present(i));
        cards_queue.push(new ThankYouCard(i));
    }

    vector<thread> threads;
    mutex mtx;

    // launch threads to process presents
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(process_presents, ref(linked_list), ref(presents_queue), ref(mtx));
    }

    // launch threads to process thank you cards
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(process_thank_you_cards, ref(linked_list), ref(cards_queue), ref(mtx));
    }

    // join threads
    for (thread& t : threads) {
        t.join();
    }

    // stopping program clock
    auto stop = std::chrono::high_resolution_clock::now();

    // Calculate the duration in microseconds
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

    cout << "Duration: " << duration.count() << " ms" << endl;

    if (linked_list.getPresentsCount() > linked_list.getThankYouCardsCount()) 
        cout << "There are more presents than 'Thank you' cards." << endl;
     else 
        cout << "There are more 'Thank you' cards than presents." << endl;
    

    return 0;
}
