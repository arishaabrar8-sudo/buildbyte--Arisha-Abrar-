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
