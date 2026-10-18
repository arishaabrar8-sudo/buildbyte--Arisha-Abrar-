// ============================================================
//  EmpowerTech C++ Backend  (all members merged into one file)
//
//  Compile:  g++ -std=c++17 -O2 -o server empowertech_backend.cpp -lsqlite3
//  (install once:  sudo apt install libsqlite3-dev)
//  Run:      ./server        (listens on http://127.0.0.1:8080)
//
//  Modules / DSA used:
//   - Course search ........ Binary Search Tree (BST)
//   - Course management .... Singly Linked List (add/delete/update/complete)
//   - Recommendations ...... Priority Queue (max-heap)
//   - Learning path ........ Graph + BFS (shortest path) and DFS (traversal)
//   - Opportunities Hub .... Merge Sort (stable, 5 sort options) + search/filter
//   - Career Center ........ File-based resume, interview questions, resources
//   - Student accounts .... Hash Table (ID -> student) + SQLite database
// ============================================================

#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <cstring>
#include <fstream>
#include <ctime>
#include <random>
#include <cstdlib>

#include <sqlite3.h>
#include <sys/time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

using namespace std;

// ==========================================
// 1. DATA STRUCTURES
// ==========================================

struct Resource {
    string name;
    string category;
    int score;
};

struct Opportunity {
    int id;
    string title;
    string type;          // Internship, Scholarship, Hackathon, Competition, Job
    string organization;
    string deadline;      // YYYY-MM-DD
    string posted;        // YYYY-MM-DD
    int amount;           // stipend / prize in PKR (0 if none)
};

struct InterviewQuestion {
    int id;
    string type;          // "Technical" or "HR"
    string question;
    string answer;
};

struct CareerResource {
    string title;
    string category;
    string description;
    string url;
};

// ==========================================
// 2. HELPER FUNCTIONS
// ==========================================

string toLowerCase(string s) {
    for (int i = 0; i < (int)s.length(); i++) {
        s[i] = tolower((unsigned char)s[i]);
    }
    return s;
}

string trim(string s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) {
        return "";
    }
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

string urlDecode(string s) {
    string result = "";
    for (int i = 0; i < (int)s.length(); i++) {
        if (s[i] == '%' && i + 2 < (int)s.length()) {
            try {
                int value = stoi(s.substr(i + 1, 2), nullptr, 16);
                result += (char)value;
                i += 2;
            }
            catch (...) {
                result += s[i];
            }
        }
        else if (s[i] == '+') {
            result += ' ';
        }
        else {
            result += s[i];
        }
    }
    return result;
}

string jsonEscape(string s) {
    string result = "";
    for (char c : s) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n";  break;
            case '\r': result += "\\r";  break;
            case '\t': result += "\\t";  break;
            default:
                if ((unsigned char)c >= 32) {
                    result += c;
                }
        }
    }
    return result;
}

// "text" -> "\"text\"" (escaped, ready for JSON)
string jstr(const string& s) {
    return "\"" + jsonEscape(s) + "\"";
}

string errorJson(const string& message) {
    return "{\"error\":" + jstr(message) + "}";
}

bool containsIgnoreCase(const string& text, const string& pattern) {
    return toLowerCase(text).find(toLowerCase(pattern)) != string::npos;
}

// Split "a|b|c" into {"a","b","c"}
vector<string> splitFields(const string& line, char delimiter) {
    vector<string> parts;
    string current;
    istringstream stream(line);
    while (getline(stream, current, delimiter)) {
        parts.push_back(current);
    }
    if (!line.empty() && line.back() == delimiter) {
        parts.push_back("");
    }
    return parts;
}

// '|' separates fields and a newline ends a record, so remove them
string cleanField(string s) {
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '|' || s[i] == '\n' || s[i] == '\r') {
            s[i] = ' ';
        }
    }
    return trim(s);
}

bool parseInt(const string& s, int& out) {
    try {
        size_t used = 0;
        int value = stoi(s, &used);
        if (used != s.size()) {
            return false;
        }
        out = value;
        return true;
    }
    catch (...) {
        return false;
    }
}

// Parse an int or return the fallback
int intOr(const string& s, int fallback) {
    int value = 0;
    if (parseInt(trim(s), value)) {
        return value;
    }
    return fallback;
}

bool fileHasData(const string& name) {
    ifstream file(name.c_str());
    return file.good() && file.peek() != ifstream::traits_type::eof();
}

string getParam(const map<string, string>& params,
                const string& key,
                const string& fallback = "") {
    map<string, string>::const_iterator it = params.find(key);
    if (it == params.end()) {
        return fallback;
    }
    return it->second;
}

// Turn "id=3&by=deadline" into {id: "3", by: "deadline"}
map<string, string> parseParams(const string& queryString) {
    map<string, string> params;
    istringstream stream(queryString);
    string part;
    while (getline(stream, part, '&')) {
        size_t equals = part.find('=');
        if (equals == string::npos) {
            continue;
        }
        params[part.substr(0, equals)] = urlDecode(part.substr(equals + 1));
    }
    return params;
}

// ------------------------------------------
// Request context (set again for every request in handleRequest)
// ------------------------------------------

string currentUserId = "guest";    // "guest" = nobody is logged in
string currentToken  = "";         // session token sent by the browser

long long nowEpoch() {
    return (long long)time(nullptr);
}

string nowDate() {
    time_t t = time(nullptr);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", localtime(&t));
    return buffer;
}

// ------------------------------------------
// HASH TABLE (separate chaining, written from scratch)
//   key   -> string (for example a student ID)
//   value -> anything (for example a Student)
// Average cost of put / get / erase is O(1).
// The table doubles in size when the load factor passes 0.75.
// ------------------------------------------

template <typename V>
class HashTable {

private:
    typedef pair<string, V> Entry;

    vector< vector<Entry> > buckets;
    size_t count;

    // djb2 hash function
    static size_t hashKey(const string& key) {

        size_t h = 5381;

        for (size_t i = 0; i < key.size(); i++) {
            h = ((h << 5) + h) + (unsigned char)key[i];
        }

        return h;
    }

    void grow() {

        vector< vector<Entry> > old;

        old.swap(buckets);

        buckets.assign(old.size() * 2, vector<Entry>());

        for (size_t b = 0; b < old.size(); b++) {
            for (size_t i = 0; i < old[b].size(); i++) {
                buckets[hashKey(old[b][i].first) % buckets.size()]
                    .push_back(old[b][i]);
            }
        }
    }

public:
    HashTable() : buckets(16), count(0) {}

    void put(const string& key, const V& value) {

        vector<Entry>& chain = buckets[hashKey(key) % buckets.size()];

        for (size_t i = 0; i < chain.size(); i++) {
            if (chain[i].first == key) {
                chain[i].second = value;
                return;
            }
        }

        chain.push_back(Entry(key, value));

        count++;

        if (count * 4 > buckets.size() * 3) {
            grow();
        }
    }

    // Returns a pointer to the value, or nullptr when the key is missing
    V* get(const string& key) {

        vector<Entry>& chain = buckets[hashKey(key) % buckets.size()];

        for (size_t i = 0; i < chain.size(); i++) {
            if (chain[i].first == key) {
                return &chain[i].second;
            }
        }

        return nullptr;
    }

    bool erase(const string& key) {

        vector<Entry>& chain = buckets[hashKey(key) % buckets.size()];

        for (size_t i = 0; i < chain.size(); i++) {
            if (chain[i].first == key) {
                chain.erase(chain.begin() + i);
                count--;
                return true;
            }
        }

        return false;
    }

    size_t size() const {
        return count;
    }

    size_t bucketCount() const {
        return buckets.size();
    }

    size_t longestChain() const {

        size_t longest = 0;

        for (size_t b = 0; b < buckets.size(); b++) {
            if (buckets[b].size() > longest) {
                longest = buckets[b].size();
            }
        }

        return longest;
    }
};

// ==========================================
// 3. COURSE DATABASE
// ==========================================

vector<Resource> resources = {
    {"AI Fundamentals", "Artificial Intelligence", 95},
    {"Cloud Computing", "Cloud", 85},
    {"Cybersecurity", "Cybersecurity", 90},
    {"Data Science", "Data Science", 92},
    {"Machine Learning", "Artificial Intelligence", 98},
    {"Programming", "Programming", 80},
    {"Web Development", "Web Development", 88}
};

// ==========================================
// 4. BINARY SEARCH (on the sorted resources vector)
//    Note: /api/resources/search now uses the BST (section 8C).
//    This function is kept as the original array-based version.
// ==========================================

int binarySearchResource(string query) {

    query = toLowerCase(trim(query));

    int left = 0;
    int right = (int)resources.size() - 1;

    while (left <= right) {

        int mid = left + (right - left) / 2;

        string current = toLowerCase(resources[mid].name);

        if (current == query) {
            return mid;
        }

        if (current < query) {
            left = mid + 1;
        }
        else {
            right = mid - 1;
        }
    }

    return -1;
}

// ==========================================
// 5. MERGE SORT (stable, with comparator)
// ==========================================

// A comparator answers: "should a come before b?"
typedef bool (*OppCompare)(const Opportunity&, const Opportunity&);

bool byDeadline(const Opportunity& a, const Opportunity& b) {
    return a.deadline < b.deadline;                        // earliest first
}

bool byPosted(const Opportunity& a, const Opportunity& b) {
    return a.posted < b.posted;                            // oldest first
}

bool byAmountDesc(const Opportunity& a, const Opportunity& b) {
    return a.amount > b.amount;                            // highest first
}

bool byType(const Opportunity& a, const Opportunity& b) {
    return toLowerCase(a.type) < toLowerCase(b.type);      // category A-Z
}

bool byTitle(const Opportunity& a, const Opportunity& b) {
    return toLowerCase(a.title) < toLowerCase(b.title);    // title A-Z
}

void merge(vector<Opportunity>& arr, int left, int mid, int right,
           OppCompare cmp) {

    vector<Opportunity> temp;

    int i = left;
    int j = mid + 1;

    // Take from the right half only when it must come first,
    // so equal items keep their order (stable sort).
    while (i <= mid && j <= right) {

        if (!cmp(arr[j], arr[i])) {
            temp.push_back(arr[i]);
            i++;
        }
        else {
            temp.push_back(arr[j]);
            j++;
        }
    }

    while (i <= mid) {
        temp.push_back(arr[i]);
        i++;
    }

    while (j <= right) {
        temp.push_back(arr[j]);
        j++;
    }

    for (int k = 0; k < (int)temp.size(); k++) {
        arr[left + k] = temp[k];
    }
}

void mergeSort(vector<Opportunity>& arr, int left, int right,
               OppCompare cmp) {

    if (left >= right) {
        return;
    }

    int mid = left + (right - left) / 2;

    mergeSort(arr, left, mid, cmp);
    mergeSort(arr, mid + 1, right, cmp);
    merge(arr, left, mid, right, cmp);
}

// ==========================================
// 6. OPPORTUNITIES HUB + CAREER CENTER (file-based "database")
// ==========================================
//
//   opportunities.txt        id|title|type|organization|deadline|posted|amount
//   saved_opportunities.txt  one opportunity id per line
//   interview_questions.txt  id|type|question|answer
//   resume_data.txt          key|value
//
// The files are created automatically on the first run, in the
// folder where the server is started.

const string OPPORTUNITIES_FILE = "opportunities.txt";
const string SAVED_FILE         = "saved_opportunities.txt";
const string QUESTIONS_FILE     = "interview_questions.txt";
const string RESUME_FILE        = "resume_data.txt";

vector<Opportunity>       opportunities;
vector<int>               savedIds;
vector<InterviewQuestion> interviewQuestions;
map<string, string>       resumeData;

const char* RESUME_KEYS[] = {
    "name", "email", "phone", "education",
    "skills", "experience", "projects"
};

const int RESUME_KEY_COUNT = 7;

vector<CareerResource> careerResources = {

    {"Resume Builder Guide", "Resume",
     "ATS friendly resume sections, tips and a checklist.", "resume.html"},

    {"Interview Preparation", "Interview",
     "Common HR questions and technical interview topics.", "interview.html"},

    {"LinkedIn Profile Guide", "Networking",
     "Build a profile that recruiters notice.", "linkedin.html"},

    {"GitHub Portfolio Guide", "Portfolio",
     "Show your projects with clean README files.", "github.html"},

    {"Job Search Guide", "Jobs",
     "Job platforms, search tips and an application checklist.", "jobs.html"},

    {"Freelancing Guide", "Freelancing",
     "Start finding clients with your technical skills.", "freelancing.html"},

    {"LeetCode", "Coding Practice",
     "Practice data structure and algorithm problems.", "https://leetcode.com"},

    {"HackerRank", "Coding Practice",
     "Coding challenges and interview preparation kits.",
     "https://www.hackerrank.com"},

    {"GeeksforGeeks", "Coding Practice",
     "Tutorials and practice for DSA and interviews.",
     "https://www.geeksforgeeks.org"},

    {"freeCodeCamp", "Learning",
     "Free courses and certifications.", "https://www.freecodecamp.org"}
};

// ------------------------------------------
// Sample data (written once, on the first run)
// ------------------------------------------

void seedOpportunitiesFile() {

    if (fileHasData(OPPORTUNITIES_FILE)) {
        return;
    }

    ofstream out(OPPORTUNITIES_FILE.c_str());

    out << "1|Backend Developer Intern|Internship|DevelopersHub|2026-11-15|2026-10-01|40000\n";
    out << "2|Software Engineering Intern|Internship|Systems Limited|2026-11-30|2026-10-05|50000\n";
    out << "3|Data Science Intern|Internship|Arbisoft|2026-10-31|2026-09-28|45000\n";
    out << "4|HEC Need Based Scholarship|Scholarship|HEC Pakistan|2026-12-15|2026-09-20|60000\n";
    out << "5|Google Women Techmakers|Scholarship|Google|2026-11-20|2026-10-02|100000\n";
    out << "6|NED Merit Scholarship|Scholarship|NED University|2026-11-05|2026-09-15|30000\n";
    out << "7|BuildByte Hackathon|Hackathon|IEEE NED|2026-11-10|2026-10-03|50000\n";
    out << "8|Karachi Hack 2026|Hackathon|Tech Valley Karachi|2026-12-01|2026-10-08|75000\n";
    out << "9|Smart Pakistan Hackathon|Hackathon|Ignite National Tech|2026-12-20|2026-10-10|100000\n";
    out << "10|ACM ICPC Regional Contest|Competition|ACM Pakistan|2026-11-25|2026-10-04|0\n";
    out << "11|National Software Competition|Competition|NASTP|2026-12-05|2026-10-06|80000\n";
    out << "12|UI/UX Design Challenge|Competition|Creative Minds PK|2026-11-18|2026-10-07|25000\n";
    out << "13|Junior Software Engineer|Job|10Pearls|2026-12-10|2026-10-09|120000\n";
    out << "14|Associate Web Developer|Job|Netsol Technologies|2026-11-28|2026-10-11|90000\n";
    out << "15|Frontend Developer Intern|Internship|Careem|2026-12-12|2026-10-12|55000\n";
}

void seedQuestionsFile() {

    if (fileHasData(QUESTIONS_FILE)) {
        return;
    }

    ofstream out(QUESTIONS_FILE.c_str());

    out << "1|Technical|What is the difference between an array and a linked list?|Arrays use contiguous memory with O(1) index access but costly insertion. Linked lists use nodes with pointers, so insertion is O(1) at a known position but access is O(n).\n";
    out << "2|Technical|Explain how Merge Sort works and its time complexity.|Divide the array in half, sort each half recursively, then merge the sorted halves. It is O(n log n) in the best, average and worst case and uses O(n) extra space.\n";
    out << "3|Technical|What is the difference between a stack and a queue?|A stack is LIFO (last in, first out). A queue is FIFO (first in, first out).\n";
    out << "4|Technical|What are the four pillars of OOP?|Encapsulation, abstraction, inheritance and polymorphism.\n";
    out << "5|Technical|What is a primary key in a database?|A column (or set of columns) that uniquely identifies each row in a table and cannot be NULL.\n";
    out << "6|Technical|What is the time complexity of binary search and what does it require?|O(log n). The data must be sorted.\n";
    out << "7|Technical|What is the difference between a pointer and a reference in C++?|A pointer can be null and reassigned. A reference must be initialised and always refers to the same object.\n";
    out << "8|HR|Tell me about yourself.|Give a short summary: your degree, key skills, one or two projects or internships, and why you are interested in this role.\n";
    out << "9|HR|Why should we hire you?|Match your skills and experience to the role and show that you learn fast and work well in a team.\n";
    out << "10|HR|What are your strengths and weaknesses?|Pick real strengths with an example. For a weakness, name something genuine and explain how you are improving it.\n";
    out << "11|HR|Describe a time you worked in a team and faced a conflict.|Use the STAR method: Situation, Task, Action, Result. Focus on how you communicated and solved it.\n";
    out << "12|HR|Where do you see yourself in five years?|Show ambition that fits the company: growing your technical skills and taking on more responsibility.\n";
}

// ------------------------------------------
// Loading and saving the "database" files
// ------------------------------------------

void loadOpportunities() {

    opportunities.clear();

    ifstream in(OPPORTUNITIES_FILE.c_str());

    string line;

    while (getline(in, line)) {

        line = trim(line);

        if (line.empty()) {
            continue;
        }

        vector<string> p = splitFields(line, '|');

        if (p.size() < 7) {
            continue;
        }

        Opportunity o;

        if (!parseInt(p[0], o.id) || !parseInt(p[6], o.amount)) {
            continue;
        }

        o.title        = p[1];
        o.type         = p[2];
        o.organization = p[3];
        o.deadline     = p[4];
        o.posted       = p[5];

        opportunities.push_back(o);
    }
}

void loadInterviewQuestions() {

    interviewQuestions.clear();

    ifstream in(QUESTIONS_FILE.c_str());

    string line;

    while (getline(in, line)) {

        line = trim(line);

        if (line.empty()) {
            continue;
        }

        vector<string> p = splitFields(line, '|');

        if (p.size() < 4) {
            continue;
        }

        InterviewQuestion q;

        if (!parseInt(p[0], q.id)) {
            continue;
        }

        q.type     = p[1];
        q.question = p[2];
        q.answer   = p[3];

        interviewQuestions.push_back(q);
    }
}

void loadSavedIds() {

    savedIds.clear();

    ifstream in(SAVED_FILE.c_str());

    string line;

    while (getline(in, line)) {

        int id = 0;

        if (parseInt(trim(line), id)) {
            savedIds.push_back(id);
        }
    }
}

void saveSavedIds() {

    ofstream out(SAVED_FILE.c_str());

    for (size_t i = 0; i < savedIds.size(); i++) {
        out << savedIds[i] << "\n";
    }
}

void loadResume() {

    resumeData.clear();

    ifstream in(RESUME_FILE.c_str());

    string line;

    while (getline(in, line)) {

        line = trim(line);

        size_t bar = line.find('|');

        if (bar == string::npos) {
            continue;
        }

        resumeData[line.substr(0, bar)] = line.substr(bar + 1);
    }
}

void saveResume() {

    ofstream out(RESUME_FILE.c_str());

    for (map<string, string>::iterator it = resumeData.begin();
         it != resumeData.end(); ++it) {

        out << it->first << "|" << it->second << "\n";
    }
}

// Called once when the server starts
void loadAllData() {

    seedOpportunitiesFile();
    seedQuestionsFile();

    loadOpportunities();
    loadInterviewQuestions();
    loadSavedIds();
    loadResume();
}

// ------------------------------------------
// Opportunities: sort, search, filter
// ------------------------------------------

// ?by=deadline | stipend | date | category | title   (nullptr = unknown)
OppCompare pickComparator(string by, string& label) {

    by = toLowerCase(trim(by));

    if (by.empty()) {
        by = "stipend";       // default: highest stipend first
    }

    label = by;

    if (by == "deadline") {
        return byDeadline;
    }

    if (by == "stipend" || by == "amount") {
        label = "stipend";
        return byAmountDesc;
    }

    if (by == "date" || by == "posted") {
        label = "date";
        return byPosted;
    }

    if (by == "category" || by == "type") {
        label = "category";
        return byType;
    }

    if (by == "title") {
        return byTitle;
    }

    return nullptr;
}

void sortOpportunities(vector<Opportunity>& list, OppCompare cmp) {

    if (list.size() > 1) {
        mergeSort(list, 0, (int)list.size() - 1, cmp);
    }
}

// field: "title", "category", "organization" or "all"
vector<Opportunity> searchOpportunities(const string& keyword,
                                        const string& field) {

    vector<Opportunity> result;

    for (size_t i = 0; i < opportunities.size(); i++) {

        const Opportunity& o = opportunities[i];

        bool match = false;

        if (field == "title" || field == "all") {
            match = match || containsIgnoreCase(o.title, keyword);
        }

        if (field == "category" || field == "all") {
            match = match || containsIgnoreCase(o.type, keyword);
        }

        if (field == "organization" || field == "all") {
            match = match || containsIgnoreCase(o.organization, keyword);
        }

        if (match) {
            result.push_back(o);
        }
    }

    return result;
}

// category = "" means "any category"
vector<Opportunity> filterOpportunities(const string& category,
                                        int minAmount) {

    vector<Opportunity> result;

    for (size_t i = 0; i < opportunities.size(); i++) {

        const Opportunity& o = opportunities[i];

        if (!category.empty() &&
            toLowerCase(o.type) != toLowerCase(category)) {
            continue;
        }

        if (o.amount < minAmount) {
            continue;
        }

        result.push_back(o);
    }

    return result;
}

bool isSaved(int id) {
    return find(savedIds.begin(), savedIds.end(), id) != savedIds.end();
}

bool opportunityExists(int id) {

    for (size_t i = 0; i < opportunities.size(); i++) {
        if (opportunities[i].id == id) {
            return true;
        }
    }

    return false;
}

// ------------------------------------------
// Opportunities: JSON builders
// ------------------------------------------

string opportunityJson(const Opportunity& o) {

    return
        "{\"id\":" + to_string(o.id) +
        ",\"title\":" + jstr(o.title) +
        ",\"type\":" + jstr(o.type) +
        ",\"organization\":" + jstr(o.organization) +
        ",\"deadline\":" + jstr(o.deadline) +
        ",\"posted\":" + jstr(o.posted) +
        ",\"amount\":" + to_string(o.amount) +
        ",\"saved\":" + string(isSaved(o.id) ? "true" : "false") +
        "}";
}

// "extra" is optional JSON that goes first, e.g. "\"sortedBy\":\"deadline\","
string opportunityListJson(const vector<Opportunity>& list,
                           const string& extra) {

    string response =
        "{" + extra +
        "\"count\":" + to_string(list.size()) +
        ",\"opportunities\":[";

    for (size_t i = 0; i < list.size(); i++) {

        if (i > 0) {
            response += ",";
        }

        response += opportunityJson(list[i]);
    }

    response += "]}";

    return response;
}

// GET /api/opportunities/sort?by=deadline|stipend|date|category|title
string opportunitiesResponse(const string& sortBy) {

    string label;

    OppCompare cmp = pickComparator(sortBy, label);

    if (cmp == nullptr) {
        return errorJson("by must be deadline, stipend, date, category or title");
    }

    vector<Opportunity> arr = opportunities;

    sortOpportunities(arr, cmp);

    return opportunityListJson(arr, "\"sortedBy\":" + jstr(label) + ",");
}

// GET /api/opportunities/search?query=intern&field=title&sort=deadline
string opportunitySearchResponse(string keyword,
                                 string field,
                                 const string& sortBy) {

    keyword = trim(keyword);

    if (keyword.empty()) {
        return errorJson("Missing 'query' parameter");
    }

    field = toLowerCase(trim(field));

    if (field.empty()) {
        field = "all";
    }

    if (field != "title" && field != "category" &&
        field != "organization" && field != "all") {

        return errorJson("field must be title, category, organization or all");
    }

    vector<Opportunity> result = searchOpportunities(keyword, field);

    if (!trim(sortBy).empty()) {

        string label;

        OppCompare cmp = pickComparator(sortBy, label);

        if (cmp == nullptr) {
            return errorJson("sort must be deadline, stipend, date, category or title");
        }

        sortOpportunities(result, cmp);
    }

    return opportunityListJson(
        result,
        "\"query\":" + jstr(keyword) +
        ",\"field\":" + jstr(field) + ","
    );
}

// GET /api/opportunities/filter?category=Hackathon&min_stipend=50000&sort=deadline
string opportunityFilterResponse(string category,
                                 const string& minStipend,
                                 const string& sortBy) {

    category = trim(category);

    int minAmount = 0;

    if (!trim(minStipend).empty() &&
        (!parseInt(trim(minStipend), minAmount) || minAmount < 0)) {

        return errorJson("min_stipend must be a whole number");
    }

    vector<Opportunity> result = filterOpportunities(category, minAmount);

    if (!trim(sortBy).empty()) {

        string label;

        OppCompare cmp = pickComparator(sortBy, label);

        if (cmp == nullptr) {
            return errorJson("sort must be deadline, stipend, date, category or title");
        }

        sortOpportunities(result, cmp);
    }

    return opportunityListJson(
        result,
        "\"category\":" + jstr(category.empty() ? "all" : category) +
        ",\"minStipend\":" + to_string(minAmount) + ","
    );
}

// GET /api/opportunities/save?id=3     (save == true)
// GET /api/opportunities/unsave?id=3   (save == false)
string saveOpportunityResponse(const string& idText, bool save) {

    int id = 0;

    if (!parseInt(trim(idText), id)) {
        return errorJson("Missing or invalid 'id' parameter");
    }

    if (!opportunityExists(id)) {
        return errorJson("No opportunity with id " + to_string(id));
    }

    string message;

    if (save) {

        if (isSaved(id)) {
            message = "Already saved";
        }
        else {
            savedIds.push_back(id);
            saveSavedIds();
            message = "Saved";
        }
    }
    else {

        vector<int>::iterator it =
            find(savedIds.begin(), savedIds.end(), id);

        if (it == savedIds.end()) {
            message = "Was not saved";
        }
        else {
            savedIds.erase(it);
            saveSavedIds();
            message = "Removed from saved";
        }
    }

    return
        "{\"success\":true,"
        "\"message\":" + jstr(message) +
        ",\"id\":" + to_string(id) +
        ",\"savedCount\":" + to_string(savedIds.size()) + "}";
}

// GET /api/opportunities/saved  (nearest deadline first)
string savedOpportunitiesResponse() {

    vector<Opportunity> result;

    for (size_t i = 0; i < opportunities.size(); i++) {
        if (isSaved(opportunities[i].id)) {
            result.push_back(opportunities[i]);
        }
    }

    sortOpportunities(result, byDeadline);

    return opportunityListJson(result, "\"sortedBy\":\"deadline\",");
}

// ------------------------------------------
// Career Center
// ------------------------------------------

string resumeJson() {

    string response = "{\"resume\":{";

    for (int i = 0; i < RESUME_KEY_COUNT; i++) {

        if (i > 0) {
            response += ",";
        }

        string key = RESUME_KEYS[i];

        map<string, string>::iterator it = resumeData.find(key);

        string value = (it == resumeData.end()) ? "" : it->second;

        response += jstr(key) + ":" + jstr(value);
    }

    response += "}}";

    return response;
}

// GET /api/career/resume/update?name=Warda&skills=C%2B%2B,Python
// Only the fields that are sent are changed.
string resumeUpdateResponse(const map<string, string>& params) {

    bool changed = false;

    for (int i = 0; i < RESUME_KEY_COUNT; i++) {

        map<string, string>::const_iterator it = params.find(RESUME_KEYS[i]);

        if (it != params.end()) {
            resumeData[RESUME_KEYS[i]] = cleanField(it->second);
            changed = true;
        }
    }

    if (!changed) {
        return errorJson(
            "Send at least one of: name, email, phone, education, "
            "skills, experience, projects"
        );
    }

    saveResume();

    return resumeJson();
}

// GET /api/career/interview?type=technical|hr|all&query=merge
string interviewResponse(string type, string keyword) {

    type = toLowerCase(trim(type));

    keyword = trim(keyword);

    if (type.empty()) {
        type = "all";
    }

    if (type != "technical" && type != "hr" && type != "all") {
        return errorJson("type must be technical, hr or all");
    }

    vector<InterviewQuestion> result;

    for (size_t i = 0; i < interviewQuestions.size(); i++) {

        const InterviewQuestion& q = interviewQuestions[i];

        if (type != "all" && toLowerCase(q.type) != type) {
            continue;
        }

        if (!keyword.empty() && !containsIgnoreCase(q.question, keyword)) {
            continue;
        }

        result.push_back(q);
    }

    string response =
        "{\"type\":" + jstr(type) +
        ",\"count\":" + to_string(result.size()) +
        ",\"questions\":[";

    for (size_t i = 0; i < result.size(); i++) {

        if (i > 0) {
            response += ",";
        }

        response +=
            "{\"id\":" + to_string(result[i].id) +
            ",\"type\":" + jstr(result[i].type) +
            ",\"question\":" + jstr(result[i].question) +
            ",\"answer\":" + jstr(result[i].answer) +
            "}";
    }

    response += "]}";

    return response;
}

// GET /api/career/resources
string careerResourcesResponse() {

    string response =
        "{\"count\":" + to_string(careerResources.size()) +
        ",\"resources\":[";

    for (size_t i = 0; i < careerResources.size(); i++) {

        if (i > 0) {
            response += ",";
        }

        response +=
            "{\"title\":" + jstr(careerResources[i].title) +
            ",\"category\":" + jstr(careerResources[i].category) +
            ",\"description\":" + jstr(careerResources[i].description) +
            ",\"url\":" + jstr(careerResources[i].url) +
            "}";
    }

    response += "]}";

    return response;
}

// Sends every /api/opportunities/... and /api/career/... request
// to the right function and sets the HTTP status for errors.
string member4Response(const string& path,
                       const map<string, string>& params,
                       string& status) {

    string body;

    if (path == "/api/opportunities/sort") {
        body = opportunitiesResponse(getParam(params, "by"));
    }
    else if (path == "/api/opportunities/search") {
        body = opportunitySearchResponse(
            getParam(params, "query"),
            getParam(params, "field", "all"),
            getParam(params, "sort")
        );
    }
    else if (path == "/api/opportunities/filter") {
        body = opportunityFilterResponse(
            getParam(params, "category"),
            getParam(params, "min_stipend"),
            getParam(params, "sort")
        );
    }
    else if (path == "/api/opportunities/save") {
        body = saveOpportunityResponse(getParam(params, "id"), true);
    }
    else if (path == "/api/opportunities/unsave") {
        body = saveOpportunityResponse(getParam(params, "id"), false);
    }
    else if (path == "/api/opportunities/saved") {
        body = savedOpportunitiesResponse();
    }
    else if (path == "/api/career/resume") {
        body = resumeJson();
    }
    else if (path == "/api/career/resume/update") {
        body = resumeUpdateResponse(params);
    }
    else if (path == "/api/career/interview") {
        body = interviewResponse(
            getParam(params, "type"),
            getParam(params, "query")
        );
    }
    else if (path == "/api/career/resources") {
        body = careerResourcesResponse();
    }
    else {
        status = "404 Not Found";
        return errorJson("Endpoint not found");
    }

    // A handler that answered {"error":...} means a bad request
    if (body.rfind("{\"error\"", 0) == 0) {
        status = "400 Bad Request";
    }

    return body;
}

// ==========================================
// 7. PRIORITY QUEUE RECOMMENDATIONS
// ==========================================

vector<Resource> getRecommendations() {

    priority_queue<pair<int, int>> pq;

    for (int i = 0; i < (int)resources.size(); i++) {
        pq.push({resources[i].score, i});
    }

    vector<Resource> result;

    while (!pq.empty()) {

        int index = pq.top().second;

        pq.pop();

        result.push_back(resources[index]);
    }

    return result;
}

// ==========================================
// 8. GRAPH + BFS AND DFS LEARNING PATH
// ==========================================

// Each course is a vertex. Each directed edge means the next course
// can be studied after the current course.
map<string, vector<string>> graph = {
    {"Programming", {"C++", "HTML", "Mathematics", "Networking"}},
    {"C++", {"Python"}},
    {"Python", {"Data Structures and Algorithms"}},
    {"Data Structures and Algorithms", {}},

    {"HTML", {"CSS"}},
    {"CSS", {"JavaScript"}},
    {"JavaScript", {"React"}},
    {"React", {"Node.js"}},
    {"Node.js", {}},

    {"Mathematics", {"Data Science"}},
    {"Data Science", {"Statistics", "Machine Learning"}},
    {"Statistics", {"Machine Learning"}},
    {"Machine Learning", {"AI Fundamentals"}},
    {"AI Fundamentals", {}},

    {"Networking", {"Cybersecurity", "Cloud Computing"}},
    {"Cybersecurity", {}},
    {"Cloud Computing", {"Cybersecurity"}}
};

// BFS uses a queue and finds a shortest route in an unweighted graph.
vector<string> findLearningPath(string start, string destination) {

    map<string, bool> visited;
    map<string, string> parent;
    queue<string> q;

    q.push(start);
    visited[start] = true;

    bool found = false;

    while (!q.empty()) {

        string current = q.front();
        q.pop();

        if (current == destination) {
            found = true;
            break;
        }

        for (string next : graph[current]) {

            if (!visited[next]) {
                visited[next] = true;
                parent[next] = current;
                q.push(next);
            }
        }
    }

    vector<string> path;

    if (!found) {
        return path;
    }

    string current = destination;

    while (current != start) {
        path.push_back(current);
        current = parent[current];
    }

    path.push_back(start);

    reverse(path.begin(), path.end());

    return path;
}

// DFS explores each reachable course deeply before backtracking.
void dfsExplore(string current, map<string, bool>& visited,
                vector<string>& result) {

    visited[current] = true;
    result.push_back(current);

    for (string next : graph[current]) {
        if (!visited[next]) {
            dfsExplore(next, visited, result);
        }
    }
}

vector<string> findDFSPath(string start) {

    map<string, bool> visited;
    vector<string> result;

    dfsExplore(start, visited, result);

    return result;
}

// Pick a start and destination node for the student's field.
bool getLearningRoute(string field, string& start, string& destination) {

    field = toLowerCase(trim(field));

    if (field == "web development" || field == "web") {
        start = "HTML";
        destination = "Node.js";
    }
    else if (field == "ai/ml" || field == "ai" ||
             field == "artificial intelligence" ||
             field == "machine learning") {
        start = "Mathematics";
        destination = "AI Fundamentals";
    }
    else if (field == "data science" || field == "data") {
        start = "Data Science";
        destination = "Statistics";
    }
    else if (field == "cybersecurity" || field == "cyber security") {
        start = "Networking";
        destination = "Cybersecurity";
    }
    else if (field == "cloud" || field == "cloud computing") {
        start = "Networking";
        destination = "Cloud Computing";
    }
    else if (field == "programming" || field == "programming fundamentals") {
        start = "Programming";
        destination = "Data Structures and Algorithms";
    }
    else {
        return false;
    }

    return true;
}

string learningPathResponse(string field, string mode) {

    string start;
    string destination;

    mode = toLowerCase(trim(mode));

    if (!getLearningRoute(field, start, destination)) {
        return errorJson(
            "Unknown learning field. Choose Web Development, AI/ML, "
            "Data Science, Cybersecurity, Cloud Computing, or Programming."
        );
    }

    vector<string> path;
    string algorithm;

    if (mode == "dfs") {
        path = findDFSPath(start);
        algorithm = "DFS";
    }
    else if (mode == "bfs" || mode.empty()) {
        path = findLearningPath(start, destination);
        algorithm = "BFS";
    }
    else {
        return errorJson("Unknown algorithm. Use bfs or dfs.");
    }

    string response =
        "{\"field\":" + jstr(field) +
        ",\"algorithm\":" + jstr(algorithm) +
        ",\"start\":" + jstr(start) +
        ",\"destination\":" + jstr(destination) +
        ",\"path\":[";

    for (int i = 0; i < (int)path.size(); i++) {

        if (i > 0) {
            response += ",";
        }

        response += jstr(path[i]);
    }

    response += "]}";

    return response;
}

// ==========================================
// 8B. LINKED LIST - DYNAMIC COURSE MANAGEMENT
// ==========================================
// Courses live in a singly linked list, so they can be added, deleted,
// updated and marked as completed at runtime.

struct CourseNode {
    Resource data;
    bool completed;
    CourseNode* next;

    CourseNode(const Resource& r)
        : data(r), completed(false), next(nullptr) {}
};

class CourseList {

private:
    CourseNode* head;
    int count;

public:
    CourseList() : head(nullptr), count(0) {}

    ~CourseList() {
        while (head != nullptr) {
            CourseNode* temp = head;
            head = head->next;
            delete temp;
        }
    }

    CourseNode* getHead() const {
        return head;
    }

    int size() const {
        return count;
    }

    // Find a node by name (case-insensitive)
    CourseNode* find(const string& name) const {

        string key = toLowerCase(trim(name));

        CourseNode* current = head;

        while (current != nullptr) {

            if (toLowerCase(current->data.name) == key) {
                return current;
            }

            current = current->next;
        }

        return nullptr;
    }

    // Add a course at the end of the list
    CourseNode* add(const Resource& r) {

        CourseNode* node = new CourseNode(r);

        if (head == nullptr) {
            head = node;
        }
        else {
            CourseNode* current = head;

            while (current->next != nullptr) {
                current = current->next;
            }

            current->next = node;
        }

        count++;

        return node;
    }

    // Delete a course by name
    bool remove(const string& name) {

        string key = toLowerCase(trim(name));

        CourseNode* current = head;
        CourseNode* previous = nullptr;

        while (current != nullptr) {

            if (toLowerCase(current->data.name) == key) {

                if (previous == nullptr) {
                    head = current->next;
                }
                else {
                    previous->next = current->next;
                }

                delete current;

                count--;

                return true;
            }

            previous = current;
            current = current->next;
        }

        return false;
    }
};

// ==========================================
// 8C. BINARY SEARCH TREE - COURSE SEARCHING
// ==========================================
// Ordered by lowercase course name. Each tree node points to the matching
// linked-list node, so search is O(log n) on average.

struct BSTNode {
    string key;
    CourseNode* course;
    BSTNode* left;
    BSTNode* right;

    BSTNode(const string& k, CourseNode* c)
        : key(k), course(c), left(nullptr), right(nullptr) {}
};

BSTNode* bstInsert(BSTNode* root, const string& key, CourseNode* course) {

    if (root == nullptr) {
        return new BSTNode(key, course);
    }

    if (key < root->key) {
        root->left = bstInsert(root->left, key, course);
    }
    else if (key > root->key) {
        root->right = bstInsert(root->right, key, course);
    }

    return root;
}

CourseNode* bstSearch(BSTNode* root, const string& key) {

    while (root != nullptr) {

        if (key == root->key) {
            return root->course;
        }

        if (key < root->key) {
            root = root->left;
        }
        else {
            root = root->right;
        }
    }

    return nullptr;
}

BSTNode* bstMinNode(BSTNode* root) {

    while (root->left != nullptr) {
        root = root->left;
    }

    return root;
}

BSTNode* bstRemove(BSTNode* root, const string& key) {

    if (root == nullptr) {
        return nullptr;
    }

    if (key < root->key) {
        root->left = bstRemove(root->left, key);
    }
    else if (key > root->key) {
        root->right = bstRemove(root->right, key);
    }
    else {

        // One child or no child
        if (root->left == nullptr) {
            BSTNode* temp = root->right;
            delete root;
            return temp;
        }

        if (root->right == nullptr) {
            BSTNode* temp = root->left;
            delete root;
            return temp;
        }

        // Two children: copy in-order successor, then delete it
        BSTNode* successor = bstMinNode(root->right);

        root->key = successor->key;
        root->course = successor->course;

        root->right = bstRemove(root->right, successor->key);
    }

    return root;
}

// In-order traversal gives courses sorted alphabetically
void bstInOrder(BSTNode* root, vector<CourseNode*>& out) {

    if (root == nullptr) {
        return;
    }

    bstInOrder(root->left, out);
    out.push_back(root->course);
    bstInOrder(root->right, out);
}

// ==========================================
// 8D. LEARNING HUB STATE (LINKED LIST + BST)
// ==========================================

CourseList courseList;

BSTNode* courseTree = nullptr;

// Fill the linked list and BST with the starting courses (called from main)
void initLearningHub() {

    for (int i = 0; i < (int)resources.size(); i++) {

        CourseNode* node = courseList.add(resources[i]);

        courseTree = bstInsert(
            courseTree,
            toLowerCase(trim(resources[i].name)),
            node
        );
    }
}

string addCourse(string name, string category, int score) {

    name = trim(name);
    category = trim(category);

    if (name.empty()) {
        return "{\"success\":false,\"message\":\"Course name is required\"}";
    }

    if (category.empty()) {
        return "{\"success\":false,\"message\":\"Category is required\"}";
    }

    if (courseList.find(name) != nullptr) {
        return "{\"success\":false,\"message\":\"Course already exists\"}";
    }

    Resource r = {name, category, score};

    CourseNode* node = courseList.add(r);

    courseTree = bstInsert(courseTree, toLowerCase(name), node);

    return "{\"success\":true,\"message\":\"Course added\"}";
}

string deleteCourse(string name) {

    string key = toLowerCase(trim(name));

    if (bstSearch(courseTree, key) == nullptr) {
        return "{\"success\":false,\"message\":\"Course not found\"}";
    }

    courseTree = bstRemove(courseTree, key);

    courseList.remove(name);

    return "{\"success\":true,\"message\":\"Course deleted\"}";
}

// Empty newName / newCategory and score < 0 mean "keep as is"
string updateCourse(string name,
                    string newName,
                    string newCategory,
                    int newScore) {

    string oldKey = toLowerCase(trim(name));

    CourseNode* node = bstSearch(courseTree, oldKey);

    if (node == nullptr) {
        return "{\"success\":false,\"message\":\"Course not found\"}";
    }

    newName = trim(newName);
    newCategory = trim(newCategory);

    if (!newName.empty()) {

        string newKey = toLowerCase(newName);

        if (newKey != oldKey && bstSearch(courseTree, newKey) != nullptr) {
            return "{\"success\":false,\"message\":\"Another course already has that name\"}";
        }

        // Name is the BST key, so re-insert under the new key
        if (newKey != oldKey) {
            courseTree = bstRemove(courseTree, oldKey);
            node->data.name = newName;
            courseTree = bstInsert(courseTree, newKey, node);
        }
        else {
            node->data.name = newName;
        }
    }

    if (!newCategory.empty()) {
        node->data.category = newCategory;
    }

    if (newScore >= 0) {
        node->data.score = newScore;
    }

    return "{\"success\":true,\"message\":\"Course updated\"}";
}

string completeCourse(string name) {

    CourseNode* node = bstSearch(courseTree, toLowerCase(trim(name)));

    if (node == nullptr) {
        return "{\"success\":false,\"message\":\"Course not found\"}";
    }

    node->completed = true;

    return "{\"success\":true,\"message\":\"Course marked as completed\"}";
}

string courseToJson(CourseNode* node) {

    return
        "{\"name\":" + jstr(node->data.name) +
        ",\"category\":" + jstr(node->data.category) +
        ",\"score\":" + to_string(node->data.score) +
        ",\"completed\":" + string(node->completed ? "true" : "false") +
        "}";
}

// ---- Learning resource roadmap (linked list) ----
// HTML -> CSS -> JavaScript -> React -> Node.js

struct RoadmapNode {
    string title;
    RoadmapNode* next;
};

string learningResourcesResponse() {

    vector<string> titles = {"HTML", "CSS", "JavaScript", "React", "Node.js"};

    RoadmapNode* head = nullptr;
    RoadmapNode* tail = nullptr;

    for (const string& t : titles) {

        RoadmapNode* node = new RoadmapNode{t, nullptr};

        if (head == nullptr) {
            head = node;
        }
        else {
            tail->next = node;
        }

        tail = node;
    }

    string response = "{\"resources\":[";

    int step = 1;

    for (RoadmapNode* cur = head; cur != nullptr; cur = cur->next) {

        if (step > 1) {
            response += ",";
        }

        response +=
            "{\"step\":" + to_string(step) +
            ",\"title\":" + jstr(cur->title) + "}";

        step++;
    }

    response += "]}";

    while (head != nullptr) {
        RoadmapNode* temp = head;
        head = head->next;
        delete temp;
    }

    return response;
}

// ==========================================
// 9. JSON RESPONSE GENERATORS
// ==========================================

// Course search uses the Binary Search Tree
string searchResponse(string query) {

    CourseNode* node = bstSearch(courseTree, toLowerCase(trim(query)));

    if (node == nullptr) {
        return "{\"found\":false,\"message\":\"Course not found\"}";
    }

    Resource r = node->data;

    return
        "{\"found\":true,"
        "\"course\":" + jstr(r.name) + ","
        "\"category\":" + jstr(r.category) + ","
        "\"score\":" + to_string(r.score) + "}";
}

string recommendationsResponse() {

    vector<Resource> result = getRecommendations();

    string response = "{\"recommendations\":[";

    for (int i = 0; i < (int)result.size(); i++) {

        if (i > 0) {
            response += ",";
        }

        response +=
            "{\"name\":" + jstr(result[i].name) +
            ",\"category\":" + jstr(result[i].category) +
            ",\"score\":" + to_string(result[i].score) +
            "}";
    }

    response += "]}";

    return response;
}

// All courses in linked-list (insertion) order
string coursesResponse() {

    string response = "{\"courses\":[";

    bool first = true;

    for (CourseNode* cur = courseList.getHead(); cur != nullptr; cur = cur->next) {

        if (!first) {
            response += ",";
        }

        first = false;

        response += courseToJson(cur);
    }

    response += "]}";

    return response;
}

// All courses alphabetically (BST in-order traversal)
string coursesSortedResponse() {

    vector<CourseNode*> ordered;

    bstInOrder(courseTree, ordered);

    string response = "{\"courses\":[";

    for (int i = 0; i < (int)ordered.size(); i++) {

        if (i > 0) {
            response += ",";
        }

        response += courseToJson(ordered[i]);
    }

    response += "]}";

    return response;
}

// Details for one course, found through the BST
string courseDetailsResponse(string name) {

    CourseNode* node = bstSearch(courseTree, toLowerCase(trim(name)));

    if (node == nullptr) {
        return "{\"found\":false,\"message\":\"Course not found\"}";
    }

    return "{\"found\":true,\"course\":" + courseToJson(node) + "}";
}

// Courses grouped by category
string categoriesResponse() {

    map<string, vector<string>> grouped;

    for (CourseNode* cur = courseList.getHead(); cur != nullptr; cur = cur->next) {
        grouped[cur->data.category].push_back(cur->data.name);
    }

    string response = "{\"categories\":[";

    bool firstCategory = true;

    for (const auto& entry : grouped) {

        if (!firstCategory) {
            response += ",";
        }

        firstCategory = false;

        response += "{\"category\":" + jstr(entry.first) + ",\"courses\":[";

        for (int i = 0; i < (int)entry.second.size(); i++) {

            if (i > 0) {
                response += ",";
            }

            response += jstr(entry.second[i]);
        }

        response += "]}";
    }

    response += "]}";

    return response;
}

// ==========================================
// 12. SQLITE DATABASE LAYER
// ==========================================
//
// File: empowertech.db (created automatically in the folder where the
// server is started). Tables:
//   students   id, name, email, salt, password_hash, joined
//   profiles   student_id, university, degree, interests, bio
//   progress   student_id, course, status (started / completed), updated
//   favorites  student_id, type (course / opportunity / resource), item
//   sessions   token, student_id, created   (who is logged in)

sqlite3* db = nullptr;

typedef vector< vector<string> > Rows;

// Runs ONE statement. Values are sent as "?" parameters (never glued into the
// SQL text), so SQL injection is not possible. SELECT results go into *rows.
bool sqlRun(const string& sql,
            const vector<string>& args = vector<string>(),
            Rows* rows = nullptr) {

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) != SQLITE_OK) {
        cerr << "SQL error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    for (size_t i = 0; i < args.size(); i++) {
        sqlite3_bind_text(statement, (int)i + 1, args[i].c_str(), -1, SQLITE_TRANSIENT);
    }

    int result = sqlite3_step(statement);

    while (result == SQLITE_ROW) {

        if (rows != nullptr) {

            vector<string> row;

            int columns = sqlite3_column_count(statement);

            for (int c = 0; c < columns; c++) {
                const unsigned char* text = sqlite3_column_text(statement, c);
                row.push_back(text ? (const char*)text : "");
            }

            rows->push_back(row);
        }

        result = sqlite3_step(statement);
    }

    if (result != SQLITE_DONE) {
        cerr << "SQL error: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(statement);
        return false;
    }

    sqlite3_finalize(statement);

    return true;
}

bool openDatabase() {

    if (sqlite3_open("empowertech.db", &db) != SQLITE_OK) {
        cerr << "Cannot open empowertech.db: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    sqlRun("PRAGMA foreign_keys = ON");

    bool ok = true;

    ok = ok && sqlRun(
        "CREATE TABLE IF NOT EXISTS students ("
        " id TEXT PRIMARY KEY,"
        " name TEXT NOT NULL,"
        " email TEXT NOT NULL UNIQUE,"
        " salt TEXT NOT NULL,"
        " password_hash TEXT NOT NULL,"
        " joined TEXT NOT NULL)");

    ok = ok && sqlRun(
        "CREATE TABLE IF NOT EXISTS profiles ("
        " student_id TEXT PRIMARY KEY REFERENCES students(id) ON DELETE CASCADE,"
        " university TEXT NOT NULL DEFAULT '',"
        " degree TEXT NOT NULL DEFAULT '',"
        " interests TEXT NOT NULL DEFAULT '',"
        " bio TEXT NOT NULL DEFAULT '')");

    ok = ok && sqlRun(
        "CREATE TABLE IF NOT EXISTS progress ("
        " student_id TEXT NOT NULL REFERENCES students(id) ON DELETE CASCADE,"
        " course TEXT NOT NULL,"
        " status TEXT NOT NULL,"
        " updated TEXT NOT NULL,"
        " PRIMARY KEY (student_id, course))");

    ok = ok && sqlRun(
        "CREATE TABLE IF NOT EXISTS favorites ("
        " student_id TEXT NOT NULL REFERENCES students(id) ON DELETE CASCADE,"
        " type TEXT NOT NULL,"
        " item TEXT NOT NULL,"
        " PRIMARY KEY (student_id, type, item))");

    ok = ok && sqlRun(
        "CREATE TABLE IF NOT EXISTS sessions ("
        " token TEXT PRIMARY KEY,"
        " student_id TEXT NOT NULL REFERENCES students(id) ON DELETE CASCADE,"
        " created INTEGER NOT NULL)");

    return ok;
}

// ==========================================
// 13. MEMBER 1 - STUDENT MANAGEMENT
//     SQLite stores the data, the HASH TABLE gives O(1) lookup
//     (student ID -> student, email -> student ID)
// ==========================================

struct Student {
    string id;
    string name;
    string email;
    string salt;
    string passwordHash;
    string joined;
};

HashTable<Student> studentsById;
HashTable<string>  emailToId;

int nextStudentNumber = 1001;

// Copy every student from SQLite into the hash tables (once, at start-up)
void loadStudentsIntoHashTable() {

    Rows rows;

    sqlRun("SELECT id, name, email, salt, password_hash, joined FROM students",
           vector<string>(), &rows);

    for (size_t i = 0; i < rows.size(); i++) {

        Student s;

        s.id           = rows[i][0];
        s.name         = rows[i][1];
        s.email        = rows[i][2];
        s.salt         = rows[i][3];
        s.passwordHash = rows[i][4];
        s.joined       = rows[i][5];

        studentsById.put(s.id, s);
        emailToId.put(s.email, s.id);

        // IDs look like EMP1001, so continue after the biggest number
        if (s.id.size() > 3 && s.id.substr(0, 3) == "EMP") {

            int number = 0;

            if (parseInt(s.id.substr(3), number) && number >= nextStudentNumber) {
                nextStudentNumber = number + 1;
            }
        }
    }
}

// ------------------------------------------
// Password hashing: PBKDF2-HMAC-SHA256 with a random salt
// (SHA-256 is written out here so the project needs no extra library)
// ------------------------------------------

typedef vector<unsigned char> Bytes;

static const unsigned int SHA_K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static unsigned int rotateRight(unsigned int x, int n) {
    return (x >> n) | (x << (32 - n));
}

Bytes sha256(const Bytes& data) {

    unsigned int h[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    Bytes message = data;

    unsigned long long bitLength = (unsigned long long)data.size() * 8;

    message.push_back(0x80);

    while (message.size() % 64 != 56) {
        message.push_back(0);
    }

    for (int i = 7; i >= 0; i--) {
        message.push_back((unsigned char)(bitLength >> (i * 8)));
    }

    for (size_t offset = 0; offset < message.size(); offset += 64) {

        unsigned int w[64];

        for (int i = 0; i < 16; i++) {
            w[i] = ((unsigned int)message[offset + i * 4]     << 24) |
                   ((unsigned int)message[offset + i * 4 + 1] << 16) |
                   ((unsigned int)message[offset + i * 4 + 2] << 8)  |
                   ((unsigned int)message[offset + i * 4 + 3]);
        }

        for (int i = 16; i < 64; i++) {

            unsigned int s0 = rotateRight(w[i - 15], 7) ^ rotateRight(w[i - 15], 18) ^ (w[i - 15] >> 3);
            unsigned int s1 = rotateRight(w[i - 2], 17) ^ rotateRight(w[i - 2], 19) ^ (w[i - 2] >> 10);

            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        unsigned int a = h[0], b = h[1], c = h[2], d = h[3];
        unsigned int e = h[4], f = h[5], g = h[6], hh = h[7];

        for (int i = 0; i < 64; i++) {

            unsigned int S1 = rotateRight(e, 6) ^ rotateRight(e, 11) ^ rotateRight(e, 25);
            unsigned int ch = (e & f) ^ (~e & g);
            unsigned int t1 = hh + S1 + ch + SHA_K[i] + w[i];
            unsigned int S0 = rotateRight(a, 2) ^ rotateRight(a, 13) ^ rotateRight(a, 22);
            unsigned int maj = (a & b) ^ (a & c) ^ (b & c);
            unsigned int t2 = S0 + maj;

            hh = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }

    Bytes digest;

    for (int i = 0; i < 8; i++) {
        digest.push_back((unsigned char)(h[i] >> 24));
        digest.push_back((unsigned char)(h[i] >> 16));
        digest.push_back((unsigned char)(h[i] >> 8));
        digest.push_back((unsigned char)(h[i]));
    }

    return digest;
}

Bytes hmacSha256(const Bytes& key, const Bytes& message) {

    Bytes k = key;

    if (k.size() > 64) {
        k = sha256(k);
    }

    k.resize(64, 0);

    Bytes inner(64), outer(64);

    for (int i = 0; i < 64; i++) {
        inner[i] = k[i] ^ 0x36;
        outer[i] = k[i] ^ 0x5c;
    }

    inner.insert(inner.end(), message.begin(), message.end());

    Bytes innerHash = sha256(inner);

    outer.insert(outer.end(), innerHash.begin(), innerHash.end());

    return sha256(outer);
}

string toHex(const Bytes& bytes) {

    static const char* digits = "0123456789abcdef";

    string text;

    for (size_t i = 0; i < bytes.size(); i++) {
        text += digits[bytes[i] >> 4];
        text += digits[bytes[i] & 15];
    }

    return text;
}

const int PBKDF2_ROUNDS = 60000;

string hashPassword(const string& password, const string& salt) {

    Bytes key(password.begin(), password.end());

    Bytes block(salt.begin(), salt.end());

    block.push_back(0);
    block.push_back(0);
    block.push_back(0);
    block.push_back(1);

    Bytes u = hmacSha256(key, block);

    Bytes result = u;

    for (int i = 1; i < PBKDF2_ROUNDS; i++) {

        u = hmacSha256(key, u);

        for (size_t j = 0; j < result.size(); j++) {
            result[j] ^= u[j];
        }
    }

    return toHex(result);
}

// Compares two strings without stopping at the first difference
bool sameText(const string& a, const string& b) {

    if (a.size() != b.size()) {
        return false;
    }

    unsigned char difference = 0;

    for (size_t i = 0; i < a.size(); i++) {
        difference |= (unsigned char)(a[i] ^ b[i]);
    }

    return difference == 0;
}

string randomHex(int byteCount) {

    static random_device source;

    Bytes bytes;

    for (int i = 0; i < byteCount; i++) {
        bytes.push_back((unsigned char)(source() & 0xFF));
    }

    return toHex(bytes);
}

// ------------------------------------------
// Sessions (stored in the sessions table)
// ------------------------------------------

const long long SESSION_SECONDS = 7LL * 24 * 60 * 60;     // 7 days

string createSession(const string& studentId) {

    string token = randomHex(24);

    sqlRun("INSERT INTO sessions (token, student_id, created) VALUES (?, ?, ?)",
           {token, studentId, to_string(nowEpoch())});

    return token;
}

// Returns the student ID for a valid token, otherwise "guest"
string userIdFromToken(const string& token) {

    if (token.empty()) {
        return "guest";
    }

    Rows rows;

    sqlRun("SELECT student_id, created FROM sessions WHERE token = ?",
           {token}, &rows);

    if (rows.empty()) {
        return "guest";
    }

    long long created = atoll(rows[0][1].c_str());

    if (nowEpoch() - created > SESSION_SECONDS) {
        sqlRun("DELETE FROM sessions WHERE token = ?", {token});
        return "guest";
    }

    return rows[0][0];
}

// ------------------------------------------
// Small helpers for the student routes
// ------------------------------------------

bool rejectUnlessPost(const string& method, string& status, string& body) {

    if (method == "POST") {
        return false;
    }

    status = "405 Method Not Allowed";
    body = errorJson("Use POST for this endpoint");

    return true;
}

bool rejectIfGuest(string& status, string& body) {

    if (currentUserId != "guest") {
        return false;
    }

    status = "401 Unauthorized";
    body = errorJson("Please log in first");

    return true;
}

bool validEmail(const string& email) {

    size_t at = email.find('@');

    if (at == string::npos || at == 0 || email.size() > 100 ||
        email.find(' ') != string::npos) {
        return false;
    }

    size_t dot = email.find('.', at);

    return dot != string::npos && dot > at + 1 && dot + 1 < email.size();
}

string studentJson(const Student& s) {

    return
        "{\"id\":" + jstr(s.id) +
        ",\"name\":" + jstr(s.name) +
        ",\"email\":" + jstr(s.email) +
        ",\"joined\":" + jstr(s.joined) + "}";
}

string profileJson(const string& studentId) {

    Rows rows;

    sqlRun("SELECT university, degree, interests, bio FROM profiles WHERE student_id = ?",
           {studentId}, &rows);

    string university, degree, interests, bio;

    if (!rows.empty()) {
        university = rows[0][0];
        degree     = rows[0][1];
        interests  = rows[0][2];
        bio        = rows[0][3];
    }

    return
        "{\"university\":" + jstr(university) +
        ",\"degree\":" + jstr(degree) +
        ",\"interests\":" + jstr(interests) +
        ",\"bio\":" + jstr(bio) + "}";
}

// 5 wrong passwords for one email = locked for 60 seconds
map<string, pair<int, long long> > loginFailures;

// ------------------------------------------
// Register / login / logout
// ------------------------------------------

// POST /api/student/register   name, email, password
string registerResponse(const map<string, string>& params, string& status) {

    string name     = trim(getParam(params, "name"));
    string email    = toLowerCase(trim(getParam(params, "email")));
    string password = getParam(params, "password");

    if (name.empty() || name.size() > 60) {
        return errorJson("Name is required (maximum 60 characters)");
    }

    if (!validEmail(email)) {
        return errorJson("Please enter a valid email address");
    }

    if (password.size() < 6 || password.size() > 100) {
        return errorJson("Password must be 6 to 100 characters");
    }

    if (emailToId.get(email) != nullptr) {
        status = "409 Conflict";
        return errorJson("This email is already registered");
    }

    Student s;

    s.id           = "EMP" + to_string(nextStudentNumber);
    s.name         = name;
    s.email        = email;
    s.salt         = randomHex(16);
    s.passwordHash = hashPassword(password, s.salt);
    s.joined       = nowDate();

    if (!sqlRun("INSERT INTO students (id, name, email, salt, password_hash, joined) "
                "VALUES (?, ?, ?, ?, ?, ?)",
                {s.id, s.name, s.email, s.salt, s.passwordHash, s.joined})) {

        status = "500 Internal Server Error";
        return errorJson("Could not save the account");
    }

    sqlRun("INSERT INTO profiles (student_id) VALUES (?)", {s.id});

    nextStudentNumber++;

    studentsById.put(s.id, s);
    emailToId.put(s.email, s.id);

    string token = createSession(s.id);

    return
        "{\"success\":true,"
        "\"token\":" + jstr(token) +
        ",\"student\":" + studentJson(s) + "}";
}

// POST /api/student/login   email, password
string loginResponse(const map<string, string>& params, string& status) {

    string email    = toLowerCase(trim(getParam(params, "email")));
    string password = getParam(params, "password");

    pair<int, long long>& failures = loginFailures[email];

    if (failures.first >= 5 && nowEpoch() < failures.second) {
        status = "429 Too Many Requests";
        return errorJson("Too many wrong attempts. Try again in a minute.");
    }

    string* idPointer = emailToId.get(email);

    Student* s = (idPointer != nullptr) ? studentsById.get(*idPointer) : nullptr;

    // The hash is always calculated so an unknown email takes the same time
    string salt = (s != nullptr) ? s->salt : string("0000000000000000");

    string attempt = hashPassword(password, salt);

    if (s == nullptr || !sameText(attempt, s->passwordHash)) {

        failures.first++;

        if (failures.first >= 5) {
            failures.second = nowEpoch() + 60;
        }

        status = "401 Unauthorized";

        return errorJson("Invalid email or password");
    }

    loginFailures.erase(email);

    string token = createSession(s->id);

    return
        "{\"success\":true,"
        "\"token\":" + jstr(token) +
        ",\"student\":" + studentJson(*s) + "}";
}

// POST /api/student/logout
string logoutResponse() {

    if (!currentToken.empty()) {
        sqlRun("DELETE FROM sessions WHERE token = ?", {currentToken});
    }

    return "{\"success\":true,\"message\":\"Logged out\"}";
}

// ------------------------------------------
// Profile
// ------------------------------------------

// GET /api/student/me
string meResponse() {

    Student* s = studentsById.get(currentUserId);

    if (s == nullptr) {
        return errorJson("Student not found");
    }

    return
        "{\"student\":" + studentJson(*s) +
        ",\"profile\":" + profileJson(s->id) + "}";
}

// POST /api/student/profile/update   name, university, degree, interests, bio
// Only the fields that are sent are changed.
string profileUpdateResponse(const map<string, string>& params) {

    Student* s = studentsById.get(currentUserId);

    if (s == nullptr) {
        return errorJson("Student not found");
    }

    if (params.count("name")) {

        string name = trim(getParam(params, "name"));

        if (name.empty() || name.size() > 60) {
            return errorJson("Name is required (maximum 60 characters)");
        }

        s->name = name;

        sqlRun("UPDATE students SET name = ? WHERE id = ?", {name, s->id});
    }

    Rows rows;

    sqlRun("SELECT university, degree, interests, bio FROM profiles WHERE student_id = ?",
           {s->id}, &rows);

    vector<string> values(4, "");

    if (!rows.empty()) {
        values = rows[0];
    }

    const char* keys[4] = {"university", "degree", "interests", "bio"};

    for (int i = 0; i < 4; i++) {

        if (params.count(keys[i])) {

            string value = trim(getParam(params, keys[i]));

            if (value.size() > 500) {
                return errorJson(string(keys[i]) + " is too long (maximum 500 characters)");
            }

            values[i] = value;
        }
    }

    sqlRun("INSERT INTO profiles (student_id, university, degree, interests, bio) "
           "VALUES (?, ?, ?, ?, ?) "
           "ON CONFLICT(student_id) DO UPDATE SET "
           "university = excluded.university, degree = excluded.degree, "
           "interests = excluded.interests, bio = excluded.bio",
           {s->id, values[0], values[1], values[2], values[3]});

    return meResponse();
}

// ------------------------------------------
// Progress and favorites
// ------------------------------------------

// POST /api/student/progress/update   course, status = started | completed | none
string progressUpdateResponse(const map<string, string>& params) {

    string course = trim(getParam(params, "course"));
    string state  = toLowerCase(trim(getParam(params, "status")));

    CourseNode* node = bstSearch(courseTree, toLowerCase(course));

    if (node == nullptr) {
        return errorJson("No course with that name");
    }

    if (state != "started" && state != "completed" && state != "none") {
        return errorJson("status must be started, completed or none");
    }

    if (state == "none") {

        sqlRun("DELETE FROM progress WHERE student_id = ? AND course = ?",
               {currentUserId, node->data.name});
    }
    else {

        sqlRun("INSERT INTO progress (student_id, course, status, updated) "
               "VALUES (?, ?, ?, ?) "
               "ON CONFLICT(student_id, course) DO UPDATE SET "
               "status = excluded.status, updated = excluded.updated",
               {currentUserId, node->data.name, state, nowDate()});
    }

    return
        "{\"success\":true,\"course\":" + jstr(node->data.name) +
        ",\"status\":" + jstr(state) + "}";
}

// GET /api/student/progress
string progressListResponse() {

    Rows rows;

    sqlRun("SELECT course, status, updated FROM progress WHERE student_id = ? ORDER BY course",
           {currentUserId}, &rows);

    string response = "{\"count\":" + to_string(rows.size()) + ",\"progress\":[";

    for (size_t i = 0; i < rows.size(); i++) {

        if (i > 0) {
            response += ",";
        }

        response +=
            "{\"course\":" + jstr(rows[i][0]) +
            ",\"status\":" + jstr(rows[i][1]) +
            ",\"updated\":" + jstr(rows[i][2]) + "}";
    }

    return response + "]}";
}

// POST /api/student/favorites/add and /remove   type, item
string favoriteChangeResponse(const map<string, string>& params, bool add) {

    string type = toLowerCase(trim(getParam(params, "type")));
    string item = trim(getParam(params, "item"));

    if (type != "course" && type != "opportunity" && type != "resource") {
        return errorJson("type must be course, opportunity or resource");
    }

    if (item.empty() || item.size() > 100) {
        return errorJson("item is required (maximum 100 characters)");
    }

    if (add) {
        sqlRun("INSERT OR IGNORE INTO favorites (student_id, type, item) VALUES (?, ?, ?)",
               {currentUserId, type, item});
    }
    else {
        sqlRun("DELETE FROM favorites WHERE student_id = ? AND type = ? AND item = ?",
               {currentUserId, type, item});
    }

    return
        "{\"success\":true,\"message\":" +
        jstr(add ? "Added to favorites" : "Removed from favorites") + "}";
}

string favoritesJsonArray() {

    Rows rows;

    sqlRun("SELECT type, item FROM favorites WHERE student_id = ? ORDER BY type, item",
           {currentUserId}, &rows);

    string response = "[";

    for (size_t i = 0; i < rows.size(); i++) {

        if (i > 0) {
            response += ",";
        }

        response += "{\"type\":" + jstr(rows[i][0]) + ",\"item\":" + jstr(rows[i][1]) + "}";
    }

    return response + "]";
}

// ------------------------------------------
// Dashboard
// ------------------------------------------

// GET /api/student/dashboard
string dashboardResponse() {

    Student* s = studentsById.get(currentUserId);

    if (s == nullptr) {
        return errorJson("Student not found");
    }

    Rows rows;

    sqlRun("SELECT course, status FROM progress WHERE student_id = ?",
           {currentUserId}, &rows);

    map<string, string> statusOf;

    int completed = 0;
    int started = 0;

    for (size_t i = 0; i < rows.size(); i++) {

        statusOf[toLowerCase(rows[i][0])] = rows[i][1];

        if (rows[i][1] == "completed") {
            completed++;
        }
        else if (rows[i][1] == "started") {
            started++;
        }
    }

    int total = courseList.size();

    int percent = (total > 0) ? (completed * 100 / total) : 0;

    // Every course with this student's status
    string courses = "[";

    bool first = true;

    for (CourseNode* cur = courseList.getHead(); cur != nullptr; cur = cur->next) {

        if (!first) {
            courses += ",";
        }

        first = false;

        string state = "none";

        map<string, string>::iterator it = statusOf.find(toLowerCase(cur->data.name));

        if (it != statusOf.end()) {
            state = it->second;
        }

        courses +=
            "{\"name\":" + jstr(cur->data.name) +
            ",\"category\":" + jstr(cur->data.category) +
            ",\"status\":" + jstr(state) + "}";
    }

    courses += "]";

    // Top 3 courses (priority queue) that the student has not finished
    vector<Resource> ranked = getRecommendations();

    string recommended = "[";

    int shown = 0;

    for (size_t i = 0; i < ranked.size() && shown < 3; i++) {

        map<string, string>::iterator it = statusOf.find(toLowerCase(ranked[i].name));

        if (it != statusOf.end() && it->second == "completed") {
            continue;
        }

        if (shown > 0) {
            recommended += ",";
        }

        recommended +=
            "{\"name\":" + jstr(ranked[i].name) +
            ",\"category\":" + jstr(ranked[i].category) +
            ",\"score\":" + to_string(ranked[i].score) + "}";

        shown++;
    }

    recommended += "]";

    return
        "{\"student\":" + studentJson(*s) +
        ",\"profile\":" + profileJson(s->id) +
        ",\"stats\":{\"completed\":" + to_string(completed) +
        ",\"started\":" + to_string(started) +
        ",\"totalCourses\":" + to_string(total) +
        ",\"percent\":" + to_string(percent) + "}" +
        ",\"courses\":" + courses +
        ",\"recommended\":" + recommended +
        ",\"favorites\":" + favoritesJsonArray() + "}";
}

// GET /api/student/lookup?id=EMP1001   (hash table lookup, public info only)
string lookupResponse(const string& idText) {

    string id = trim(idText);

    for (size_t i = 0; i < id.size(); i++) {
        id[i] = (char)toupper((unsigned char)id[i]);
    }

    Student* s = studentsById.get(id);

    if (s == nullptr) {
        return "{\"found\":false,\"message\":\"No student with that ID\"}";
    }

    return
        "{\"found\":true,\"id\":" + jstr(s->id) +
        ",\"name\":" + jstr(s->name) + "}";
}

// Routes /api/student/...
string studentRouter(const string& path,
                     const string& method,
                     const map<string, string>& params,
                     string& status) {

    string body;

    if (path == "/api/student/register") {

        if (rejectUnlessPost(method, status, body)) return body;

        return registerResponse(params, status);
    }

    if (path == "/api/student/login") {

        if (rejectUnlessPost(method, status, body)) return body;

        return loginResponse(params, status);
    }

    // Everything below needs a logged-in student
    if (rejectIfGuest(status, body)) return body;

    if (path == "/api/student/logout") {

        if (rejectUnlessPost(method, status, body)) return body;

        return logoutResponse();
    }

    if (path == "/api/student/me") {
        return meResponse();
    }

    if (path == "/api/student/dashboard") {
        return dashboardResponse();
    }

    if (path == "/api/student/profile/update") {

        if (rejectUnlessPost(method, status, body)) return body;

        return profileUpdateResponse(params);
    }

    if (path == "/api/student/progress") {
        return progressListResponse();
    }

    if (path == "/api/student/progress/update") {

        if (rejectUnlessPost(method, status, body)) return body;

        return progressUpdateResponse(params);
    }

    if (path == "/api/student/favorites") {
        return "{\"favorites\":" + favoritesJsonArray() + "}";
    }

    if (path == "/api/student/favorites/add" || path == "/api/student/favorites/remove") {

        if (rejectUnlessPost(method, status, body)) return body;

        return favoriteChangeResponse(params, path == "/api/student/favorites/add");
    }

    if (path == "/api/student/lookup") {
        return lookupResponse(getParam(params, "id"));
    }

    status = "404 Not Found";

    return errorJson("Endpoint not found");
}

// GET /api/system/hash-stats  (shows how the hash table is doing)
string hashStatsResponse() {

    return
        "{\"students\":{\"count\":" + to_string(studentsById.size()) +
        ",\"buckets\":" + to_string(studentsById.bucketCount()) +
        ",\"longestChain\":" + to_string(studentsById.longestChain()) + "}}";
}

// ==========================================
// 10. HTTP REQUEST HANDLING
// ==========================================

string getHeaderValue(const string& request, const string& headerName) {

    istringstream input(request);

    string line;

    string target = toLowerCase(headerName);

    while (getline(input, line)) {

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        size_t colon = line.find(':');

        if (colon == string::npos) {
            continue;
        }

        string name = toLowerCase(line.substr(0, colon));

        if (name == target) {
            return trim(line.substr(colon + 1));
        }
    }

    return "";
}

// Reads one complete HTTP request: headers first, then the body
// (as many bytes as Content-Length says). Needed for POST requests.
string readHttpRequest(int clientSocket) {

    string data;

    char buffer[4096];

    size_t headerEnd = string::npos;
    size_t totalNeeded = 0;

    while (data.size() < 65536) {

        ssize_t received = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (received <= 0) {
            break;
        }

        data.append(buffer, (size_t)received);

        if (headerEnd == string::npos) {

            headerEnd = data.find("\r\n\r\n");

            if (headerEnd != string::npos) {

                int length = 0;

                parseInt(getHeaderValue(data.substr(0, headerEnd), "Content-Length"), length);

                if (length < 0 || length > 20000) {
                    length = 0;
                }

                totalNeeded = headerEnd + 4 + (size_t)length;
            }
        }

        if (headerEnd != string::npos && data.size() >= totalNeeded) {
            break;
        }
    }

    return data;
}

string handleRequest(string request) {

    istringstream input(request);

    string method;
    string target;
    string version;

    input >> method >> target >> version;

    // Permit requests only from the local frontend server.
    string origin = getHeaderValue(request, "Origin");

    string corsHeaders = "";

    if (origin == "http://127.0.0.1:5500") {
        corsHeaders =
            "Access-Control-Allow-Origin: " + origin + "\r\n"
            "Vary: Origin\r\n";
    }

    if (method == "OPTIONS") {

        return
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
            "Access-Control-Max-Age: 600\r\n" +
            corsHeaders +
            "Connection: close\r\n"
            "Content-Length: 0\r\n\r\n";
    }

    // Who is asking?  "Authorization: Bearer <token>"
    string authorization = getHeaderValue(request, "Authorization");

    currentToken = "";

    if (authorization.size() > 7 && authorization.substr(0, 7) == "Bearer ") {
        currentToken = trim(authorization.substr(7));
    }

    currentUserId = userIdFromToken(currentToken);

    string body;
    string status = "200 OK";

    if (method != "GET" && method != "POST") {

        status = "405 Method Not Allowed";

        body = "{\"error\":\"Only GET and POST requests are supported\"}";
    }
    else {

        size_t question = target.find('?');

        string path = target.substr(0, question);

        string queryString = "";

        if (question != string::npos) {
            queryString = target.substr(question + 1);
        }

        // Parameters from the URL ...
        map<string, string> params = parseParams(queryString);

        // ... and from the body of a POST (name=Ali&email=...)
        if (method == "POST") {

            size_t bodyStart = request.find("\r\n\r\n");

            if (bodyStart != string::npos) {

                map<string, string> bodyParams =
                    parseParams(request.substr(bodyStart + 4));

                for (map<string, string>::iterator it = bodyParams.begin();
                     it != bodyParams.end(); ++it) {
                    params[it->first] = it->second;
                }
            }
        }

        // ---- Learning Hub / core routes ----

        if (path == "/api/resources/search") {
            body = searchResponse(getParam(params, "query"));
        }

        else if (path == "/api/learning-path") {
            // ?field=Web Development&mode=bfs|dfs   (defaults: AI/ML, bfs)
            body = learningPathResponse(
                getParam(params, "field", "AI/ML"),
                getParam(params, "mode", "bfs")
            );
        }

        else if (path == "/api/recommendations") {
            body = recommendationsResponse();
        }

        else if (path == "/api/health") {
            body =
                "{\"status\":\"running\","
                "\"backend\":\"C++\","
                "\"database\":\"SQLite\","
                "\"framework\":\"None\"}";
        }

        else if (path == "/api/courses") {
            body = coursesResponse();
        }

        else if (path == "/api/courses/sorted") {
            body = coursesSortedResponse();
        }

        else if (path == "/api/courses/details") {
            body = courseDetailsResponse(getParam(params, "name"));
        }

        // /api/courses/add?name=X&category=Y&score=80
        else if (path == "/api/courses/add") {
            body = addCourse(
                getParam(params, "name"),
                getParam(params, "category"),
                intOr(getParam(params, "score"), 50)
            );
        }

        else if (path == "/api/courses/delete") {
            body = deleteCourse(getParam(params, "name"));
        }

        // /api/courses/update?name=X&newName=Y&category=Z&score=90
        else if (path == "/api/courses/update") {
            body = updateCourse(
                getParam(params, "name"),
                getParam(params, "newName"),
                getParam(params, "category"),
                intOr(getParam(params, "score"), -1)
            );
        }

        else if (path == "/api/courses/complete") {
            body = completeCourse(getParam(params, "name"));
        }

        else if (path == "/api/categories") {
            body = categoriesResponse();
        }

        else if (path == "/api/learning-resources") {
            body = learningResourcesResponse();
        }

        // ---- Opportunities Hub + Career Center ----

        else if (path.rfind("/api/opportunities/", 0) == 0 ||
                 path.rfind("/api/career/", 0) == 0) {

            body = member4Response(path, params, status);
        }

        // ---- Student accounts (SQLite + hash table) ----

        else if (path.rfind("/api/student/", 0) == 0) {
            body = studentRouter(path, method, params, status);
        }

        else if (path == "/api/system/hash-stats") {
            body = hashStatsResponse();
        }

        else {
            status = "404 Not Found";
            body = "{\"error\":\"Endpoint not found\"}";
        }
    }

    // Any {"error":...} that did not set its own status is a bad request
    if (status == "200 OK" && body.rfind("{\"error\"", 0) == 0) {
        status = "400 Bad Request";
    }

    string response =
        "HTTP/1.1 " + status + "\r\n"
        "Content-Type: application/json; charset=utf-8\r\n" +
        corsHeaders +
        "Cache-Control: no-store\r\n"
        "Connection: close\r\n"
        "Content-Length: " + to_string(body.size()) +
        "\r\n\r\n" +
        body;

    return response;
}

// ==========================================
// 11. START THE C++ HTTP SERVER
// ==========================================

int main() {

    // Load files (opportunities, questions, saved, resume) and
    // build the linked list + BST of courses
    loadAllData();
    initLearningHub();

    // SQLite database + hash tables for the student accounts
    if (!openDatabase()) {
        return 1;
    }

    loadStudentsIntoHashTable();

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket < 0) {
        cerr << "Error creating socket.\n";
        return 1;
    }

    int reuse = 1;

    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR,
               &reuse, sizeof(reuse));

    sockaddr_in serverAddress{};

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);

    // Listen only on the local machine
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(serverSocket, (sockaddr*)&serverAddress,
             sizeof(serverAddress)) < 0) {

        cerr << "Error binding port 8080.\n";
        cerr << "Check whether another program is using it.\n";

        close(serverSocket);

        return 1;
    }

    if (listen(serverSocket, 10) < 0) {

        cerr << "Error starting server.\n";

        close(serverSocket);

        return 1;
    }

    cout << "\n====================================\n";
    cout << "       EmpowerTech C++ Backend\n";
    cout << "====================================\n";

    cout << "Server: http://127.0.0.1:8080\n";

    cout << "\nCore:\n";
    cout << "/api/health\n";
    cout << "/api/resources/search?query=Machine%20Learning\n";
    cout << "/api/recommendations\n";
    cout << "/api/learning-path?field=Web%20Development&mode=bfs\n";
    cout << "/api/learning-path?field=AI/ML&mode=dfs\n";

    cout << "\nLearning Hub (Linked List + BST):\n";
    cout << "/api/courses\n";
    cout << "/api/courses/sorted\n";
    cout << "/api/courses/details?name=Programming\n";
    cout << "/api/courses/add?name=Python&category=Programming&score=90\n";
    cout << "/api/courses/update?name=Python&score=95\n";
    cout << "/api/courses/complete?name=Python\n";
    cout << "/api/courses/delete?name=Python\n";
    cout << "/api/categories\n";
    cout << "/api/learning-resources\n";

    cout << "\nOpportunities Hub:\n";
    cout << "/api/opportunities/sort?by=deadline\n";
    cout << "/api/opportunities/search?query=intern&field=title\n";
    cout << "/api/opportunities/filter?category=Hackathon&sort=deadline\n";
    cout << "/api/opportunities/save?id=3\n";
    cout << "/api/opportunities/unsave?id=3\n";
    cout << "/api/opportunities/saved\n";

    cout << "\nStudent accounts (SQLite):\n";
    cout << "POST /api/student/register   (name, email, password)\n";
    cout << "POST /api/student/login      (email, password)\n";
    cout << "POST /api/student/logout\n";
    cout << "GET  /api/student/me\n";
    cout << "GET  /api/student/dashboard\n";
    cout << "POST /api/student/profile/update\n";
    cout << "POST /api/student/progress/update\n";
    cout << "GET  /api/student/favorites  |  POST .../favorites/add  .../favorites/remove\n";
    cout << "GET  /api/student/lookup?id=EMP1001\n";
    cout << "GET  /api/system/hash-stats\n";

    cout << "\nCareer Center:\n";
    cout << "/api/career/resume\n";
    cout << "/api/career/resume/update?name=Warda\n";
    cout << "/api/career/interview?type=hr\n";
    cout << "/api/career/resources\n";

    cout << "\nServer is running. Press Ctrl+C to stop.\n\n";

    while (true) {

        sockaddr_in clientAddress{};

        socklen_t clientLength = sizeof(clientAddress);

        int clientSocket = accept(serverSocket,
                                  (sockaddr*)&clientAddress,
                                  &clientLength);

        if (clientSocket < 0) {
            continue;
        }

        // Do not wait forever for a slow or broken client
        timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO,
                   &timeout, sizeof(timeout));

        string request = readHttpRequest(clientSocket);

        if (!request.empty()) {

            string response = handleRequest(request);

            size_t sent = 0;

            while (sent < response.size()) {

                ssize_t result = send(clientSocket,
                                      response.data() + sent,
                                      response.size() - sent, 0);

                if (result <= 0) {
                    break;
                }

                sent += (size_t)result;
            }
        }

        close(clientSocket);
    }

    close(serverSocket);

    return 0;
}
