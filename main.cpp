#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using Paise = long long;
constexpr Paise maxBalance = 1'000'000'000'000LL;

struct Roommate {
    std::string name, room;
    bool current = true;
    Paise balance = 0; // Positive means they should receive money.
};

struct Share {
    std::size_t person;
    Paise amount;
};

struct Expense {
    std::string category;
    Paise amount;
    std::size_t payer;
    std::vector<Share> shares;
};

class Apartment {
    std::vector<Roommate> people_;
    std::vector<Expense> expenses_;

public:
    const auto& people() const { return people_; }
    const auto& expenses() const { return expenses_; }

    void addRoommate(const std::string& name, const std::string& room) {
        if (name.find_first_not_of(" \t") == std::string::npos ||
            room.find_first_not_of(" \t") == std::string::npos)
            throw std::invalid_argument("Enter a name and room number.");
        people_.push_back({name, room});
    }

    void removeRoommate(std::size_t person) {
        people_.at(person).current = false;
    }

    void addExpense(const std::string& category, Paise amount, std::size_t payer) {
        if (amount <= 0 || !people_.at(payer).current)
            throw std::invalid_argument("Enter a positive amount and choose a current roommate.");
        const auto count = std::count_if(people_.begin(), people_.end(),
            [](const Roommate& person) { return person.current; });
        Paise leftover = amount % count;
        Expense expense{category, amount, payer, {}};
        for (std::size_t i = 0; i < people_.size(); ++i) {
            if (!people_[i].current) continue;
            Paise share = amount / count;
            if (leftover > 0) {
                ++share;
                --leftover;
            }
            expense.shares.push_back({i, share});
        }

        for (const auto& share : expense.shares) {
            const Paise change = (share.person == payer ? amount : 0) - share.amount;
            const Paise balance = people_[share.person].balance + change;
            if (balance < -maxBalance || balance > maxBalance)
                throw std::invalid_argument("This expense would push a balance above the supported limit.");
        }

        expenses_.push_back(expense);
        for (const auto& share : expense.shares)
            people_[share.person].balance += (share.person == payer ? amount : 0) - share.amount;
    }

    void recordPayment(std::size_t from, std::size_t to, Paise amount) {
        auto& payer = people_.at(from);
        auto& receiver = people_.at(to);
        if (from == to || amount <= 0 || payer.balance > -amount || receiver.balance < amount)
            throw std::invalid_argument("That amount exceeds what the payer owes or the recipient is owed.");
        payer.balance += amount;
        receiver.balance -= amount;
    }

    static Apartment starter() {
        Apartment home;
        home.addRoommate("Aarav Sharma", "1");
        home.addRoommate("Ananya Rao", "2");
        home.addRoommate("Kabir Singh", "3");
        home.addExpense("Rent", 1'800'000, 0);
        home.addExpense("Electricity", 237'000, 1);
        home.addExpense("Water", 54'000, 2);
        home.addExpense("Wi-Fi", 99'900, 2);
        home.addExpense("Groceries", 168'000, 1);
        home.addExpense("Cleaning supplies", 36'000, 0);
        return home;
    }

    void save(const std::filesystem::path& path) const {
        auto temp = path;
        temp += ".tmp";
        std::ofstream file(temp);
        file.exceptions(std::ios::failbit | std::ios::badbit);
        file << "ROOMMATES1 " << people_.size() << '\n';
        for (const auto& person : people_) {
            file << std::quoted(person.name) << ' ' << std::quoted(person.room) << ' '
                 << person.current << ' ' << person.balance << '\n';
        }
        file << expenses_.size() << '\n';
        for (const auto& expense : expenses_) {
            file << std::quoted(expense.category) << ' ' << expense.amount << ' '
                 << expense.payer << ' ' << expense.shares.size();
            for (const auto& share : expense.shares)
                file << ' ' << share.person << ' ' << share.amount;
            file << '\n';
        }
        file << "END\n";
        file.close();
        std::filesystem::rename(temp, path);
    }

    static Apartment load(const std::filesystem::path& path) {
        if (!std::filesystem::exists(path)) return starter();

        Apartment home;
        std::ifstream file(path);
        std::string marker;
        std::size_t count;
        if (!(file >> marker >> count) || marker != "ROOMMATES1")
            throw std::runtime_error("Couldn't open the saved household.");

        for (std::size_t i = 0; i < count; ++i) {
            Roommate person;
            if (!(file >> std::quoted(person.name) >> std::quoted(person.room)
                       >> person.current >> person.balance) ||
                person.balance < -maxBalance || person.balance > maxBalance)
                throw std::runtime_error("Invalid saved roommate.");
            home.people_.push_back(person);
        }

        if (!(file >> count)) throw std::runtime_error("Missing saved expenses.");
        for (std::size_t i = 0; i < count; ++i) {
            Expense expense;
            std::size_t shares;
            if (!(file >> std::quoted(expense.category) >> expense.amount >> expense.payer >> shares) ||
                expense.payer >= home.people_.size() || shares == 0 || shares > home.people_.size() ||
                expense.amount <= 0)
                throw std::runtime_error("Invalid saved expense.");
            Paise remaining = expense.amount;
            for (std::size_t j = 0; j < shares; ++j) {
                Share share;
                if (!(file >> share.person >> share.amount) || share.person >= home.people_.size() ||
                    share.amount < 0 || share.amount > remaining)
                    throw std::runtime_error("Invalid saved expense share.");
                expense.shares.push_back(share);
                remaining -= share.amount;
            }
            if (remaining != 0) throw std::runtime_error("Saved shares do not add up.");
            home.expenses_.push_back(expense);
        }

        if (!(file >> marker) || marker != "END" || file >> marker)
            throw std::runtime_error("The saved household is incomplete.");
        return home;
    }
};

struct EndOfInput {};

std::string ask(const char* prompt) {
    std::cout << prompt;
    std::string answer;
    if (!std::getline(std::cin, answer)) throw EndOfInput{};
    return answer;
}

std::size_t choose(const std::string& answer, std::size_t first, std::size_t last) {
    if (answer.empty() || answer.size() > 9 ||
        answer.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("Choose one of the numbers shown.");
    const auto number = std::stoul(answer);
    if (number < first || number > last)
        throw std::invalid_argument("Choose one of the numbers shown.");
    return number;
}

Paise readAmount() {
    const auto text = ask("Amount in rupees (e.g. 499.50): ");
    const auto dot = text.find('.');
    const auto rupees = text.substr(0, dot);
    const auto paise = dot == std::string::npos ? "" : text.substr(dot + 1);
    if (rupees.empty() || rupees.size() > 9 || paise.size() > 2 ||
        (dot != std::string::npos && paise.empty()) ||
        rupees.find_first_not_of("0123456789") != std::string::npos ||
        paise.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("Enter an amount like 499 or 499.50.");
    const Paise fraction = paise.empty() ? 0 : std::stoll(paise) * (paise.size() == 1 ? 10 : 1);
    return std::stoll(rupees) * 100 + fraction;
}

std::string money(Paise amount) {
    const auto paise = amount % 100;
    return "Rs. " + std::to_string(amount / 100) + "." +
           (paise < 10 ? "0" : "") + std::to_string(paise);
}

std::vector<std::size_t> currentRoommates(const Apartment& home) {
    std::vector<std::size_t> result;
    for (std::size_t i = 0; i < home.people().size(); ++i)
        if (home.people()[i].current) result.push_back(i);
    return result;
}

void showRoommates(const Apartment& home, const std::vector<std::size_t>& list) {
    for (std::size_t i = 0; i < list.size(); ++i) {
        const auto& person = home.people()[list[i]];
        std::cout << "  " << i + 1 << ". " << person.name << " (Room " << person.room << ")\n";
    }
}

std::size_t pickRoommate(const Apartment& home, const std::vector<std::size_t>& list,
                         const char* prompt) {
    if (list.empty()) throw std::invalid_argument("Add a roommate first.");
    showRoommates(home, list);
    return list[choose(ask(prompt), 1, list.size()) - 1];
}

void showExpense(const Apartment& home, const Expense& expense) {
    std::cout << expense.category << " | " << money(expense.amount)
              << " | Paid by " << home.people()[expense.payer].name << '\n';
    for (const auto& share : expense.shares)
        std::cout << "  " << home.people()[share.person].name << "'s share: " << money(share.amount) << '\n';
}

void addExpense(Apartment& home) {
    const auto roster = currentRoommates(home);
    if (roster.empty()) throw std::invalid_argument("Add a roommate first.");
    const std::vector<std::string> categories{"Rent", "Electricity", "Water", "Wi-Fi", "Cooking gas",
        "Groceries", "Cleaning supplies", "Repairs and maintenance", "Other"};
    for (std::size_t i = 0; i < categories.size(); ++i)
        std::cout << "  " << i + 1 << ". " << categories[i] << '\n';
    const auto category = choose(ask("What was it for? "), 1, categories.size()) - 1;
    const auto payer = pickRoommate(home, roster, "Who paid? ");
    home.addExpense(categories[category], readAmount(), payer);
    showExpense(home, home.expenses().back());
}

void showSettlements(const Apartment& home) {
    std::vector<Paise> balances;
    for (const auto& person : home.people()) balances.push_back(person.balance);
    bool unsettled = false;
    for (std::size_t from = 0; from < balances.size(); ++from) {
        for (std::size_t to = 0; to < balances.size() && balances[from] < 0; ++to) {
            if (balances[to] <= 0) continue;
            const auto amount = std::min(-balances[from], balances[to]);
            std::cout << home.people()[from].name << " owes " << home.people()[to].name
                      << ' ' << money(amount) << '\n';
            balances[from] += amount;
            balances[to] -= amount;
            unsettled = true;
        }
    }
    if (!unsettled) std::cout << "Everyone is settled up.\n";
}

bool recordPayment(Apartment& home) {
    std::vector<std::size_t> debtors, creditors;
    for (std::size_t i = 0; i < home.people().size(); ++i) {
        if (home.people()[i].balance < 0) debtors.push_back(i);
        if (home.people()[i].balance > 0) creditors.push_back(i);
    }
    if (debtors.empty()) {
        std::cout << "Everyone is settled up.\n";
        return false;
    }
    const auto payer = pickRoommate(home, debtors, "Who sent the money? ");
    const auto receiver = pickRoommate(home, creditors, "Who received it? ");
    home.recordPayment(payer, receiver, readAmount());
    std::cout << "Payment recorded.\n";
    showSettlements(home);
    return true;
}

int main(int argc, char* argv[]) {
    const std::filesystem::path path = argc > 1 ? argv[1] : "household.txt";
    try {
        auto home = Apartment::load(path);
        std::cout << "Roommate Expense Splitter\n";
        while (true) {
            std::cout << R"(
1. Add roommate
2. Remove roommate
3. View roommates
4. Add expense
5. View expenses
6. See who owes whom
7. Record a payment
0. Exit
)";
            try {
                const auto action = choose(ask("Pick an option: "), 0, 7);
                bool changed = false;
                switch (action) {
                    case 0: return 0;
                    case 1: {
                        const auto name = ask("What's their name? ");
                        const auto room = ask("Which room? ");
                        home.addRoommate(name, room);
                        std::cout << name << " joined the apartment.\n";
                        changed = true;
                        break;
                    }
                    case 2: {
                        const auto person = pickRoommate(home, currentRoommates(home), "Who is moving out? ");
                        home.removeRoommate(person);
                        std::cout << home.people()[person].name << " was removed from the roster.\n";
                        changed = true;
                        break;
                    }
                    case 3: {
                        const auto roster = currentRoommates(home);
                        if (roster.empty()) std::cout << "No roommates yet.\n";
                        else showRoommates(home, roster);
                        break;
                    }
                    case 4:
                        addExpense(home);
                        changed = true;
                        break;
                    case 5:
                        if (home.expenses().empty()) std::cout << "No expenses yet.\n";
                        for (const auto& expense : home.expenses()) showExpense(home, expense);
                        break;
                    case 6:
                        showSettlements(home);
                        break;
                    case 7:
                        changed = recordPayment(home);
                        break;
                }
                if (changed) {
                    try {
                        home.save(path);
                    } catch (const std::exception&) {
                        std::cout << "Couldn't save. Your changes are available for this session.\n";
                    }
                }
            } catch (const std::invalid_argument& error) {
                std::cout << error.what() << '\n';
            }
        }
    } catch (const EndOfInput&) {
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
