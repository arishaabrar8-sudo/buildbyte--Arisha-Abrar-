#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <cstring>

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
    string title;
    string type;
    int amount;
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

void merge(vector<Opportunity>& arr,
           int left,
           int mid,
           int right) {

    vector<Opportunity> temp;

    int i = left;
    int j = mid + 1;

    // Merge in descending order of amount
    while (i <= mid && j <= right) {

        if (arr[i].amount >= arr[j].amount) {
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
               int right) {

    if (left >= right) {
        return;
    }

    int mid = left + (right - left) / 2;

    mergeSort(arr, left, mid);

    mergeSort(arr, mid + 1, right);

    merge(arr, left, mid, right);
}

// ==========================================
// 6. SCHOLARSHIPS AND INTERNSHIPS
// ==========================================

vector<Opportunity> opportunities = {

    {"Software Engineering Internship",
     "Internship", 30000},

    {"Merit Scholarship",
     "Scholarship", 100000},

    {"AI Research Internship",
     "Internship", 45000},

    {"Need-Based Scholarship",
     "Scholarship", 75000},

    {"Web Development Internship",
     "Internship", 25000}
};

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

// ##########################################################
// >>> NEW CODE START (Member 2 - Learning Hub: Linked List + BST)
// ##########################################################

// ==========================================
// 8B. LINKED LIST - DYNAMIC COURSE / RESOURCE MANAGEMENT   [NEW]
// ==========================================
// Every course lives in a singly linked list, so courses can be
// added, deleted, updated and marked as completed at runtime
// without any fixed-size array.

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
// 8C. BINARY SEARCH TREE - COURSE SEARCHING   [NEW]
// ==========================================
// The BST is ordered by lowercase course name. Each tree node points to
// the matching linked-list node, so search is O(log n) on average.

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
// 8D. LEARNING HUB STATE (LINKED LIST + BST)   [NEW]
// ==========================================

CourseList courseList;

BSTNode* courseTree = nullptr;

// Fill the linked list and BST with the starting courses
bool initLearningHub() {

    for (int i = 0; i < (int)resources.size(); i++) {

        CourseNode* node = courseList.add(resources[i]);

        courseTree = bstInsert(
            courseTree,
            toLowerCase(trim(resources[i].name)),
            node
        );
    }

    return true;
}

// Runs once at program start, so main() does not need to change
static bool learningHubReady = initLearningHub();

// Add a new course (linked list + BST)
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

// Delete a course (linked list + BST)
string deleteCourse(string name) {

    string key = toLowerCase(trim(name));

    if (bstSearch(courseTree, key) == nullptr) {
        return "{\"success\":false,\"message\":\"Course not found\"}";
    }

    courseTree = bstRemove(courseTree, key);

    courseList.remove(name);

    return "{\"success\":true,\"message\":\"Course deleted\"}";
}

// Update a course; empty newName / newCategory and score < 0 mean "keep as is"
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

// Mark a course as completed
string completeCourse(string name) {

    CourseNode* node = bstSearch(courseTree, toLowerCase(trim(name)));

    if (node == nullptr) {
        return "{\"success\":false,\"message\":\"Course not found\"}";
    }

    node->completed = true;

    return "{\"success\":true,\"message\":\"Course marked as completed\"}";
}

// JSON for a single course node
string courseToJson(CourseNode* node) {

    return
        "{\"name\":\"" + jsonEscape(node->data.name) +
        "\",\"category\":\"" + jsonEscape(node->data.category) +
        "\",\"score\":" + to_string(node->data.score) +
        ",\"completed\":" + (node->completed ? "true" : "false") +
        "}";
}

// ---- Learning resource roadmap (linked list) ----
// HTML -> CSS -> JavaScript -> React -> Node.js

struct RoadmapNode {
    string title;
    RoadmapNode* next;
};

string learningResourcesResponse() {

    vector<string> titles = {
        "HTML", "CSS", "JavaScript", "React", "Node.js"
    };

    // Build the chain
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

    // Walk the chain
    string response = "{\"resources\":[";

    int step = 1;

    for (RoadmapNode* cur = head; cur != nullptr; cur = cur->next) {

        if (step > 1) {
            response += ",";
        }

        response +=
            "{\"step\":" + to_string(step) +
            ",\"title\":\"" + jsonEscape(cur->title) + "\"}";

        step++;
    }

    response += "]}";

    // Free the chain
    while (head != nullptr) {
        RoadmapNode* temp = head;
        head = head->next;
        delete temp;
    }

    return response;
}

// ##########################################################
// >>> NEW CODE END (sections 8B - 8D)
// ##########################################################

// ==========================================
// 9. JSON RESPONSE GENERATORS
// ==========================================

// >>> CHANGED: course search now uses the Binary Search Tree
// >>> (previously used binarySearchResource). JSON output is identical.
string searchResponse(string query) {

    CourseNode* node =
        bstSearch(courseTree, toLowerCase(trim(query)));

    if (node == nullptr) {

        return
            "{\"found\":false,"
            "\"message\":\"Course not found\"}";
    }

    Resource r = node->data;

    return
        "{\"found\":true,"
        "\"course\":\"" + jsonEscape(r.name) + "\","
        "\"category\":\"" + jsonEscape(r.category) + "\","
        "\"score\":" + to_string(r.score) + "}";
}

string opportunitiesResponse() {

    vector<Opportunity> arr = opportunities;

    mergeSort(arr, 0, (int)arr.size() - 1);

    string response = "{\"opportunities\":[";

    for (int i = 0; i < (int)arr.size(); i++) {

        if (i > 0) {
            response += ",";
        }

        response +=
            "{\"title\":\"" +
            jsonEscape(arr[i].title) +

            "\",\"type\":\"" +
            jsonEscape(arr[i].type) +

            "\",\"amount\":" +
            to_string(arr[i].amount) +

            "}";
    }

    response += "]}";

    return response;
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

// >>> NEW CODE START: Learning Hub responses (linked list + BST)

// All courses in linked-list (insertion) order
string coursesResponse() {

    string response = "{\"courses\":[";

    bool first = true;

    for (CourseNode* cur = courseList.getHead();
         cur != nullptr;
         cur = cur->next) {

        if (!first) {
            response += ",";
        }

        first = false;

        response += courseToJson(cur);
    }

    response += "]}";

    return response;
}

// All courses alphabetically using BST in-order traversal
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

    CourseNode* node =
        bstSearch(courseTree, toLowerCase(trim(name)));

    if (node == nullptr) {
        return
            "{\"found\":false,"
            "\"message\":\"Course not found\"}";
    }

    return "{\"found\":true,\"course\":" + courseToJson(node) + "}";
}

// Courses grouped by category
string categoriesResponse() {

    map<string, vector<string>> grouped;

    for (CourseNode* cur = courseList.getHead();
         cur != nullptr;
         cur = cur->next) {

        grouped[cur->data.category].push_back(cur->data.name);
    }

    string response = "{\"categories\":[";

    bool firstCategory = true;

    for (const auto& entry : grouped) {

        if (!firstCategory) {
            response += ",";
        }

        firstCategory = false;

        response +=
            "{\"category\":\"" + jsonEscape(entry.first) +
            "\",\"courses\":[";

        for (int i = 0; i < (int)entry.second.size(); i++) {

            if (i > 0) {
                response += ",";
            }

            response += "\"" + jsonEscape(entry.second[i]) + "\"";
        }

        response += "]}";
    }

    response += "]}";

    return response;
}

// >>> NEW CODE END: Learning Hub responses

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

        // >>> NEW: stores all query parameters (used by the Learning Hub routes)
        map<string, string> params;

        // Extract the query parameter
        // >>> CHANGED: stream renamed from "params" to "paramStream"
        // >>> so the new "params" map above does not clash with it
        istringstream paramStream(queryString);

        string part;

        while (getline(paramStream, part, '&')) {

            size_t equals = part.find('=');

            if (equals == string::npos) {
                continue;
            }

            string key = part.substr(0, equals);

            string value = part.substr(equals + 1);

            // >>> NEW: save every parameter
            params[key] = urlDecode(value);

            if (key == "query") {
                query = urlDecode(value);
            }
        }

        // Route 1: Course search
        if (path == "/api/resources/search") {

            body = searchResponse(query);
        }

        // Route 2: Sort opportunities
        else if (path == "/api/opportunities/sort") {

            body = opportunitiesResponse();
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

        // >>> NEW CODE START: Learning Hub routes (Linked List + BST)

        // List all courses (linked list)
        else if (path == "/api/courses") {

            body = coursesResponse();
        }

        // List all courses alphabetically (BST in-order)
        else if (path == "/api/courses/sorted") {

            body = coursesSortedResponse();
        }

        // Course details (BST search)
        else if (path == "/api/courses/details") {

            body = courseDetailsResponse(params["name"]);
        }

        // Add course: /api/courses/add?name=X&category=Y&score=80
        else if (path == "/api/courses/add") {

            int score = 50;

            if (params.count("score")) {
                try {
                    score = stoi(params["score"]);
                }
                catch (...) {
                    score = 50;
                }
            }

            body = addCourse(params["name"], params["category"], score);
        }

        // Delete course: /api/courses/delete?name=X
        else if (path == "/api/courses/delete") {

            body = deleteCourse(params["name"]);
        }

        // Update course:
        // /api/courses/update?name=X&newName=Y&category=Z&score=90
        else if (path == "/api/courses/update") {

            int score = -1;

            if (params.count("score")) {
                try {
                    score = stoi(params["score"]);
                }
                catch (...) {
                    score = -1;
                }
            }

            body = updateCourse(
                params["name"],
                params["newName"],
                params["category"],
                score
            );
        }

        // Mark completed: /api/courses/complete?name=X
        else if (path == "/api/courses/complete") {

            body = completeCourse(params["name"]);
        }

        // Course categories
        else if (path == "/api/categories") {

            body = categoriesResponse();
        }

        // Learning resources chain (linked list)
        else if (path == "/api/learning-resources") {

            body = learningResourcesResponse();
        }

        // >>> NEW CODE END: Learning Hub routes

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

    // >>> NEW: startup messages for the Learning Hub endpoints
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
    // >>> END NEW

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
