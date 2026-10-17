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
    string type;          // category: Internship, Scholarship, Hackathon, Competition, Job
    string organization;
    string deadline;      // YYYY-MM-DD
    string posted;        // YYYY-MM-DD (date posted)
    int amount;           // stipend / prize in PKR (0 if none)
};

// ==========================================
// 2. HELPER FUNCTIONS
// ==========================================

// Convert a string to lowercase
string toLowerCase(string s) {
    for (int i = 0; i < (int)s.length(); i++) {
        s[i] = tolower((unsigned char)s[i]);
    }

    return s;
}

// Remove spaces from the beginning and end
string trim(string s) {
    size_t start = s.find_first_not_of(" \t\r\n");

    if (start == string::npos) {
        return "";
    }

    size_t end = s.find_last_not_of(" \t\r\n");

    return s.substr(start, end - start + 1);
}

// Decode URL parameters
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

// Escape strings before placing them in JSON
string jsonEscape(string s) {
    string result = "";

    for (char c : s) {
        switch (c) {
            case '"':
                result += "\\\"";
                break;

            case '\\':
                result += "\\\\";
                break;

            case '\n':
                result += "\\n";
                break;

            case '\r':
                result += "\\r";
                break;

            case '\t':
                result += "\\t";
                break;

            default:
                if ((unsigned char)c >= 32) {
                    result += c;
                }
        }
    }

    return result;
}

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
// 4. BINARY SEARCH
// ==========================================

int binarySearchResource(string query) {

    query = toLowerCase(trim(query));

    int left = 0;
    int right = (int)resources.size() - 1;

    while (left <= right) {

        int mid = left + (right - left) / 2;

        string current =
            toLowerCase(resources[mid].name);

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
// 5. MERGE SORT
// ==========================================

// A comparator answers: "should a come before b?"
typedef bool (*OppCompare)(const Opportunity&, const Opportunity&);
 
bool byDeadline(const Opportunity& a, const Opportunity& b) {
    return a.deadline < b.deadline;            // earliest first
}
 
bool byPosted(const Opportunity& a, const Opportunity& b) {
    return a.posted < b.posted;                // oldest first
}
 
bool byAmountDesc(const Opportunity& a, const Opportunity& b) {
    return a.amount > b.amount;                // highest stipend first
}
 
bool byType(const Opportunity& a, const Opportunity& b) {
    return toLowerCase(a.type) < toLowerCase(b.type);     // category A-Z
}
 
bool byTitle(const Opportunity& a, const Opportunity& b) {
    return toLowerCase(a.title) < toLowerCase(b.title);   // title A-Z
}
 
void merge(vector<Opportunity>& arr,
           int left,
           int mid,
           int right,
           OppCompare cmp) {
 
    vector<Opportunity> temp;
 
    int i = left;
    int j = mid + 1;
 
    // Take from the right half only when it must come first.
    // Equal items keep their order, so the sort is stable.
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
 
void mergeSort(vector<Opportunity>& arr,
               int left,
               int right,
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
// 6. SCHOLARSHIPS AND INTERNSHIPS
// ==========================================

//
// "Database" = plain text files in the folder where the server is started.
// They are created automatically on the first run.
//
//   opportunities.txt        id|title|type|organization|deadline|posted|amount
//   saved_opportunities.txt  one opportunity id per line
//   interview_questions.txt  id|type|question|answer      (type: Technical / HR)
//   resume_data.txt          key|value
//
// DSA used in this module:
//   - Merge Sort (section 5) for every "sort by" option
//   - Linear search, case-insensitive substring match (title / category / organization)
//   - Filtering by category and minimum stipend
//
// Note: "type" in Opportunity is the category
// (Internship, Scholarship, Hackathon, Competition, Job).
 
struct InterviewQuestion {
    int id;
    string type;        // "Technical" or "HR"
    string question;
    string answer;
};
 
struct CareerResource {
    string title;
    string category;
    string description;
    string url;         // a page of our website, or an external link
};
 
const string OPPORTUNITIES_FILE = "opportunities.txt";
const string SAVED_FILE         = "saved_opportunities.txt";
const string QUESTIONS_FILE     = "interview_questions.txt";
const string RESUME_FILE        = "resume_data.txt";
 
vector<Opportunity>       opportunities;       // table: opportunities
vector<int>               savedIds;            // table: saved_opportunities
vector<InterviewQuestion> interviewQuestions;  // table: interview_questions
map<string, string>       resumeData;          // table: resume_data
 
const char* RESUME_KEYS[] = {
    "name", "email", "phone", "education",
    "skills", "experience", "projects"
};
 
const int RESUME_KEY_COUNT = 7;
 
vector<CareerResource> careerResources = {
 
    {"Resume Builder Guide", "Resume",
     "ATS friendly resume sections, tips and a checklist.",
     "resume.html"},
 
    {"Interview Preparation", "Interview",
     "Common HR questions and technical interview topics.",
     "interview.html"},
 
    {"LinkedIn Profile Guide", "Networking",
     "Build a profile that recruiters notice.",
     "linkedin.html"},
 
    {"GitHub Portfolio Guide", "Portfolio",
     "Show your projects with clean README files.",
     "github.html"},
 
    {"Job Search Guide", "Jobs",
     "Job platforms, search tips and an application checklist.",
     "jobs.html"},
 
    {"Freelancing Guide", "Freelancing",
     "Start finding clients with your technical skills.",
     "freelancing.html"},
 
    {"LeetCode", "Coding Practice",
     "Practice data structure and algorithm problems.",
     "https://leetcode.com"},
 
    {"HackerRank", "Coding Practice",
     "Coding challenges and interview preparation kits.",
     "https://www.hackerrank.com"},
 
    {"GeeksforGeeks", "Coding Practice",
     "Tutorials and practice for DSA and interviews.",
     "https://www.geeksforgeeks.org"},
 
    {"freeCodeCamp", "Learning",
     "Free courses and certifications.",
     "https://www.freecodecamp.org"}
};
 
// ------------------------------------------
// Small helpers for this module
// ------------------------------------------
 
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
 
// '|' separates fields and a newline ends a record, so they
// must never appear inside text that is saved to a file.
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
 
bool fileHasData(const string& name) {
 
    ifstream file(name.c_str());
 
    return file.good() &&
           file.peek() != ifstream::traits_type::eof();
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
 
// Turn "id=3&by=deadline" into a map {id: "3", by: "deadline"}
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
 
string quoted(const string& s) {
 
    return "\"" + jsonEscape(s) + "\"";
}
 
string errorJson(const string& message) {
 
    return "{\"error\":" + quoted(message) + "}";
}
 
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
 
// Choose the comparator for ?by=deadline | stipend | date | category | title
// Returns nullptr for an unknown option. "label" is the cleaned-up name.
OppCompare pickComparator(string by, string& label) {
 
    by = toLowerCase(trim(by));
 
    if (by.empty()) {
        by = "stipend";       // default, same as the original route
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
// JSON builders
// ------------------------------------------
 
string opportunityJson(const Opportunity& o) {
 
    return
        "{\"id\":" + to_string(o.id) +
        ",\"title\":" + quoted(o.title) +
        ",\"type\":" + quoted(o.type) +
        ",\"organization\":" + quoted(o.organization) +
        ",\"deadline\":" + quoted(o.deadline) +
        ",\"posted\":" + quoted(o.posted) +
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
        "\"query\":" + quoted(keyword) +
        ",\"field\":" + quoted(field) + ","
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
        "\"category\":" + quoted(category.empty() ? "all" : category) +
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
        "\"message\":" + quoted(message) +
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
 
        response += quoted(key) + ":" + quoted(value);
    }
 
    response += "}}";
 
    return response;
}
 
// GET /api/career/resume
string resumeResponse() {
 
    return resumeJson();
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
        "{\"type\":" + quoted(type) +
        ",\"count\":" + to_string(result.size()) +
        ",\"questions\":[";
 
    for (size_t i = 0; i < result.size(); i++) {
 
        if (i > 0) {
            response += ",";
        }
 
        response +=
            "{\"id\":" + to_string(result[i].id) +
            ",\"type\":" + quoted(result[i].type) +
            ",\"question\":" + quoted(result[i].question) +
            ",\"answer\":" + quoted(result[i].answer) +
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
            "{\"title\":" + quoted(careerResources[i].title) +
            ",\"category\":" + quoted(careerResources[i].category) +
            ",\"description\":" + quoted(careerResources[i].description) +
            ",\"url\":" + quoted(careerResources[i].url) +
            "}";
    }
 
    response += "]}";
 
    return response;
}

// ==========================================
// 7. PRIORITY QUEUE RECOMMENDATIONS
// ==========================================

vector<Resource> getRecommendations() {

    priority_queue<pair<int, int>> pq;

    for (int i = 0; i < (int)resources.size(); i++) {

        pq.push({
            resources[i].score,
            i
        });
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
// 8. GRAPH + BFS LEARNING PATH
// ==========================================

map<string, vector<string>> graph = {

    {"Programming",
     {"Web Development", "Data Science"}},

    {"Web Development",
     {"Cloud Computing"}},

    {"Data Science",
     {"Machine Learning"}},

    {"Machine Learning",
     {"AI Fundamentals"}},

    {"Cloud Computing",
     {"Cybersecurity"}},

    {"Cybersecurity", {}},

    {"AI Fundamentals", {}}
};

vector<string> findLearningPath(
    string start,
    string destination
) {

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

// ==========================================
// 9. JSON RESPONSE GENERATORS
// ==========================================

string searchResponse(string query) {

    int index = binarySearchResource(query);

    if (index == -1) {

        return
            "{\"found\":false,"
            "\"message\":\"Course not found\"}";
    }

    Resource r = resources[index];

    return
        "{\"found\":true,"
        "\"course\":\"" + jsonEscape(r.name) + "\","
        "\"category\":\"" + jsonEscape(r.category) + "\","
        "\"score\":" + to_string(r.score) + "}";
}

// GET /api/opportunities/sort?by=deadline | stipend | date | category | title
// With no ?by= it sorts by stipend (highest first), as before.
string opportunitiesResponse(const string& sortBy) {
 
    string label;
 
    OppCompare cmp = pickComparator(sortBy, label);
 
    if (cmp == nullptr) {
        return errorJson("by must be deadline, stipend, date, category or title");
    }
 
    vector<Opportunity> arr = opportunities;
 
    sortOpportunities(arr, cmp);
 
    return opportunityListJson(arr, "\"sortedBy\":" + quoted(label) + ",");
}
 
// Member 4: sends every /api/opportunities/... and /api/career/... request
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
 
        body = resumeResponse();
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
string recommendationsResponse() {

    vector<Resource> result = getRecommendations();

    string response = "{\"recommendations\":[";

    for (int i = 0; i < (int)result.size(); i++) {

        if (i > 0) {
            response += ",";
        }

        response +=
            "{\"name\":\"" +
            jsonEscape(result[i].name) +

            "\",\"category\":\"" +
            jsonEscape(result[i].category) +

            "\",\"score\":" +
            to_string(result[i].score) +

            "}";
    }

    response += "]}";

    return response;
}

string learningPathResponse() {

    vector<string> path = findLearningPath(
        "Programming",
        "AI Fundamentals"
    );

    string response = "{\"path\":[";

    for (int i = 0; i < (int)path.size(); i++) {

        if (i > 0) {
            response += ",";
        }

        response += "\"" + jsonEscape(path[i]) + "\"";
    }

    response += "]}";

    return response;
}

// ==========================================
// 10. HTTP REQUEST HANDLING
// ==========================================

string getHeaderValue(
    const string& request,
    const string& headerName
) {

    istringstream input(request);

    string line;

    string target = headerName;

    transform(
        target.begin(),
        target.end(),
        target.begin(),
        [](unsigned char c) {
            return tolower(c);
        }
    );

    while (getline(input, line)) {

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        size_t colon = line.find(':');

        if (colon == string::npos) {
            continue;
        }

        string name = line.substr(0, colon);

        transform(
            name.begin(),
            name.end(),
            name.begin(),
            [](unsigned char c) {
                return tolower(c);
            }
        );

        if (name == target) {
            return trim(line.substr(colon + 1));
        }
    }

    return "";
}

string handleRequest(string request) {

    istringstream input(request);

    string method;
    string target;
    string version;

    input >> method >> target >> version;

    // Permit requests from the local frontend server.
    // This is intentionally restricted to the expected origin.
    string origin = getHeaderValue(request, "Origin");

    string allowedOrigin = "";

    if (origin == "http://127.0.0.1:5500") {
        allowedOrigin = origin;
    }

    string corsHeaders = "";

    if (!allowedOrigin.empty()) {

        corsHeaders =
            "Access-Control-Allow-Origin: " +
            allowedOrigin + "\r\n"
            "Vary: Origin\r\n";
    }

    if (method == "OPTIONS") {

        return
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Methods: GET, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n" +
            corsHeaders +
            "Connection: close\r\n"
            "Content-Length: 0\r\n\r\n";
    }

    string body;
    string status = "200 OK";

    if (method != "GET") {

        status = "405 Method Not Allowed";

        body =
            "{\"error\":\"Only GET requests are supported\"}";
    }

    else {

        size_t question = target.find('?');

        string path = target.substr(0, question);

        string queryString = "";

        if (question != string::npos) {
            queryString = target.substr(question + 1);
        }

        string query = "";

        // Extract the query parameter
        istringstream params(queryString);

        string part;

        while (getline(params, part, '&')) {

            size_t equals = part.find('=');

            if (equals == string::npos) {
                continue;
            }

            string key = part.substr(0, equals);

            string value = part.substr(equals + 1);

            if (key == "query") {
                query = urlDecode(value);
            }
        }

        // Route 1: Course search
        if (path == "/api/resources/search") {

            body = searchResponse(query);
        }

        // Route 2: Opportunities Hub + Career Center (Member 4)
        else if (path.rfind("/api/opportunities/", 0) == 0 ||
                 path.rfind("/api/career/", 0) == 0) {
 
            body = member4Response(path, parseParams(queryString), status);
        }

        // Route 3: Learning path
        else if (path == "/api/learning-path") {

            body = learningPathResponse();
        }

        // Route 4: Recommendations
        else if (path == "/api/recommendations") {

            body = recommendationsResponse();
        }

        // Route 5: Health check
        else if (path == "/api/health") {

            body =
                "{\"status\":\"running\","
                "\"backend\":\"C++\","
                "\"framework\":\"None\"}";
        }

        else {

            status = "404 Not Found";

            body = "{\"error\":\"Endpoint not found\"}";
        }
    }

    string response =
        "HTTP/1.1 " + status + "\r\n"
        "Content-Type: application/json; charset=utf-8\r\n" +
        corsHeaders +
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
    loadAllData();

    int serverSocket = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (serverSocket < 0) {

        cerr << "Error creating socket.\n";

        return 1;
    }

    int reuse = 1;

    setsockopt(
        serverSocket,
        SOL_SOCKET,
        SO_REUSEADDR,
        &reuse,
        sizeof(reuse)
    );

    sockaddr_in serverAddress{};

    serverAddress.sin_family = AF_INET;

    serverAddress.sin_port = htons(8080);

    // Listen only on the local machine
    serverAddress.sin_addr.s_addr =
        inet_addr("127.0.0.1");

    if (bind(
        serverSocket,
        (sockaddr*)&serverAddress,
        sizeof(serverAddress)
    ) < 0) {

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

    cout << "\nAvailable endpoints:\n";

    cout << "/api/health\n";
    cout << "/api/resources/search?query=Machine%20Learning\n";
    cout << "/api/opportunities/sort\n";
    cout << "/api/learning-path\n";
    cout << "/api/recommendations\n";
    cout << "\nOpportunities Hub:\n";
    cout << "/api/opportunities/sort?by=deadline\n";
    cout << "/api/opportunities/search?query=intern&field=title\n";
    cout << "/api/opportunities/filter?category=Hackathon&sort=deadline\n";
    cout << "/api/opportunities/save?id=3\n";
    cout << "/api/opportunities/unsave?id=3\n";
    cout << "/api/opportunities/saved\n";
 
    cout << "\nCareer Center:\n";
    cout << "/api/career/resume\n";
    cout << "/api/career/resume/update?name=Warda\n";
    cout << "/api/career/interview?type=hr\n";
    cout << "/api/career/resources\n";

    cout << "\nServer is running. Press Ctrl+C to stop.\n\n";

    while (true) {

        sockaddr_in clientAddress{};

        socklen_t clientLength =
            sizeof(clientAddress);

        int clientSocket = accept(
            serverSocket,
            (sockaddr*)&clientAddress,
            &clientLength
        );

        if (clientSocket < 0) {
            continue;
        }

        char buffer[8192] = {};

        ssize_t bytesReceived = recv(
            clientSocket,
            buffer,
            sizeof(buffer) - 1,
            0
        );

        if (bytesReceived > 0) {

            string request(
                buffer,
                (size_t)bytesReceived
            );

            string response = handleRequest(request);

            size_t sent = 0;

            while (sent < response.size()) {

                ssize_t result = send(
                    clientSocket,
                    response.data() + sent,
                    response.size() - sent,
                    0
                );

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
