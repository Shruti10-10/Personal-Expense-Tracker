// simple_expense_tracker_multiuser.cpp
// Single shared file with username field + simple salted password auth (college-demo)

#include <bits/stdc++.h>
using namespace std;

const string DATA_FILE = "expenses.txt";   // id|username|date|amount|category|notes
const string USERS_FILE = "users.txt";     // username|salt|hash

struct Expense {
    int id;
    string username;
    string date;    // YYYY-MM-DD
    double amount;
    string category;
    string notes;
};

string current_user;

// ---------- small helpers ----------
vector<string> split(const string &s, char delim) {
    vector<string> out;
    string cur;
    for(char c: s){
        if(c == delim){ out.push_back(cur); cur.clear(); }
        else cur.push_back(c);
    }
    out.push_back(cur);
    return out;
}

string now_date() {
    time_t t = time(nullptr);
    tm tm = *localtime(&t);
    char buf[16];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
    return string(buf);
}

// Very small salted hash using std::hash for demo purposes only.
// NOT production secure. Replace with proper crypto (bcrypt/Argon2) for real apps.
string simple_hash(const string &salt, const string &password){
    // combine salt + password
    string comb = salt + password;
    size_t h = std::hash<string>{}(comb);
    // produce hex-like string
    stringstream ss; ss << std::hex << h;
    return ss.str();
}

string make_salt(){
    // create a short random hex salt
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<uint64_t> dist;
    uint64_t v = dist(gen);
    stringstream ss; ss << hex << v;
    return ss.str();
}

// ---------- user auth ----------
bool user_exists(const string &username) {
    ifstream f(USERS_FILE);
    string line;
    while(getline(f, line)){
        if(line.empty()) continue;
        auto p = split(line,'|');
        if(p.size() >= 1 && p[0] == username) return true;
    }
    return false;
}

bool register_user(const string &username, const string &password) {
    if(username.empty()) return false;
    if(user_exists(username)) return false;
    string salt = make_salt();
    string h = simple_hash(salt, password);
    ofstream f(USERS_FILE, ios::app);
    f << username << "|" << salt << "|" << h << "\n";
    return true;
}

bool login_user(const string &username, const string &password) {
    ifstream f(USERS_FILE);
    string line;
    while(getline(f, line)){
        if(line.empty()) continue;
        auto p = split(line,'|');
        if(p.size() < 3) continue;
        if(p[0] == username){
            string salt = p[1];
            string stored = p[2];
            string h = simple_hash(salt, password);
            return (h == stored);
        }
    }
    return false;
}

void prompt_login(){
    while(true){
        cout << "Welcome — choose an option:\n1) Login\n2) Register\n3) Quit\nChoice: ";
        string c; getline(cin, c);
        if(c == "1"){
            cout << "Username: "; string u; getline(cin, u);
            cout << "Password: "; string p; getline(cin, p);
            if(login_user(u,p)){
                current_user = u;
                cout << "Logged in as " << current_user << "\n";
                return;
            } else {
                cout << "Invalid username or password.\n";
            }
        } else if(c == "2"){
            cout << "Choose username: "; string u; getline(cin, u);
            if(u.empty()){ cout<<"Username cannot be empty.\n"; continue; }
            if(user_exists(u)){ cout << "User exists.\n"; continue; }
            cout << "Choose password: "; string p; getline(cin, p);
            if(register_user(u,p)){
                cout << "Registered. You can now login.\n";
            } else {
                cout << "Registration failed.\n";
            }
        } else if(c == "3"){
            cout << "Bye.\n";
            exit(0);
        } else {
            cout << "Invalid choice.\n";
        }
    }
}

// ---------- expenses I/O ----------
vector<Expense> load_expenses() {
    vector<Expense> res;
    ifstream f(DATA_FILE);
    string line;
    while(getline(f, line)){
        if(line.empty()) continue;
        auto p = split(line, '|');
        if(p.size() < 6) continue; // id|username|date|amount|category|notes
        Expense e;
        try {
            e.id = stoi(p[0]);
        } catch(...) { continue; }
        e.username = p[1];
        e.date = p[2];
        try { e.amount = stod(p[3]); } catch(...) { e.amount = 0.0; }
        e.category = p[4];
        e.notes = p[5];
        res.push_back(e);
    }
    return res;
}

void save_expenses(const vector<Expense>& exps){
    ofstream f(DATA_FILE, ios::trunc);
    for(const auto &e: exps){
        // ensure no '|' in notes/fields or you'll break parsing
        f << e.id << "|" << e.username << "|" << e.date << "|" << e.amount << "|" << e.category << "|" << e.notes << "\n";
    }
}

int next_id(const vector<Expense>& exps){
    int m = 0;
    for(const auto &e: exps) if(e.id > m) m = e.id;
    return m + 1;
}

// ---------- UI actions (user-scoped) ----------
void add_expense(){
    auto exps = load_expenses(); // global file
    Expense e;
    e.id = next_id(exps);
    e.username = current_user;
    cout << "Date (YYYY-MM-DD) ["<< now_date() << "]: ";
    string s; getline(cin, s);
    e.date = s.empty() ? now_date() : s;
    cout << "Amount: "; getline(cin, s);
    try { e.amount = stod(s); } catch(...) { cout << "Invalid amount. Aborted.\n"; return; }
    cout << "Category (e.g., food,rent): "; getline(cin, s); e.category = s.empty() ? "uncategorized" : s;
    cout << "Notes (avoid '|'): "; getline(cin, s); e.notes = s;
    exps.push_back(e);
    save_expenses(exps);
    cout << "Added (id=" << e.id << ").\n";
}

void list_expenses(){
    auto exps = load_expenses();
    vector<Expense> mine;
    for(auto &e: exps) if(e.username == current_user) mine.push_back(e);
    if(mine.empty()){ cout << "No expenses found for " << current_user << ".\n"; return; }
    cout << left << setw(5) << "ID" << setw(12) << "Date" << setw(10) << "Amount" << setw(15) << "Category" << "Notes\n";
    cout << string(70,'-') << "\n";
    for(auto &e: mine){
        cout << setw(5) << e.id << setw(12) << e.date << setw(10) << e.amount << setw(15) << e.category << e.notes << "\n";
    }
}

void delete_expense(){
    auto exps = load_expenses();
    vector<Expense> mine;
    for(auto &e: exps) if(e.username == current_user) mine.push_back(e);
    if(mine.empty()){ cout << "No expenses to delete.\n"; return; }
    list_expenses();
    cout << "Enter ID to delete (only your own IDs): ";
    string s; getline(cin, s);
    if(s.empty()) return;
    int id = 0;
    try { id = stoi(s); } catch(...) { cout << "Invalid ID.\n"; return; }
    // find in global list an element with matching id AND username
    auto it = find_if(exps.begin(), exps.end(), [&](const Expense &x){ return x.id == id && x.username == current_user; });
    if(it == exps.end()){ cout << "ID not found or you don't have permission.\n"; return; }
    exps.erase(it);
    save_expenses(exps);
    cout << "Deleted.\n";
}

void monthly_summary(){
    auto exps = load_expenses();
    cout << "Enter month (YYYY-MM) [current month]: ";
    string s; getline(cin, s);
    string month = s.empty() ? now_date().substr(0,7) : s;
    double total = 0.0;
    map<string,double> cat;
    for(auto &e: exps){
        if(e.username != current_user) continue;
        if(e.date.size() >= 7 && e.date.substr(0,7) == month){
            total += e.amount;
            cat[e.category] += e.amount;
        }
    }
    cout << "Summary for " << current_user << " in " << month << ":\n";
    cout << "Total expense: " << total << "\n";
    cout << "By category:\n";
    for(auto &p: cat) cout << "  " << p.first << " : " << p.second << "\n";
}

void export_text(){
    auto exps = load_expenses();
    string out = "export_" + current_user + "_" + now_date() + ".txt";
    ofstream f(out);
    for(auto &e: exps){
        if(e.username != current_user) continue;
        f << e.id << "\t" << e.date << "\t" << e.amount << "\t" << e.category << "\t" << e.notes << "\n";
    }
    cout << "Exported to " << out << "\n";
}

// ---------- main ----------
int main(){
    cin.tie(nullptr);
    cout << "Simple Expense Tracker (multi-user)\n";
    prompt_login(); // sets current_user or exits

    while(true){
        cout << "\nMenu:\n1) Add expense\n2) List expenses\n3) Delete expense\n4) Monthly summary\n5) Export to text\n6) Logout\n7) Exit\nChoose: ";
        string c; getline(cin, c);
        if(c=="1") add_expense();
        else if(c=="2") list_expenses();
        else if(c=="3") delete_expense();
        else if(c=="4") monthly_summary();
        else if(c=="5") export_text();
        else if(c=="6") { cout << "Logging out...\n"; current_user.clear(); prompt_login(); }
        else if(c=="7") { cout << "Bye.\n"; break; }
        else cout << "Invalid.\n";
    }
    return 0;
}
