// =====================================================
// EMPOWERTECH - script.js
// Original features are kept. Backend features were added
// at the bottom of each section / in the BACKEND section.
// =====================================================

console.log("JavaScript Connected");

// =====================================================
// BACKEND HELPERS (used by search, chatbot and the pages below)
// Open your pages with Live Server at http://127.0.0.1:5500
// =====================================================

const API_BASE_URL = "http://127.0.0.1:8080";

// Calls the C++ backend and returns the parsed JSON.
// Returns null if the server is not running.
// The login token is kept in the browser so the student stays logged in
const TOKEN_KEY = "empowertech_token";

function getToken() {
    try { return localStorage.getItem(TOKEN_KEY) || ""; }
    catch (error) { return ""; }
}

function setToken(token) {
    try {
        if (token) { localStorage.setItem(TOKEN_KEY, token); }
        else { localStorage.removeItem(TOKEN_KEY); }
    } catch (error) { /* storage blocked: student must log in again */ }
}

// method "GET": params go in the URL.  method "POST": params go in the body
// (used for passwords, so they never appear in the address bar).
async function api(path, params = {}, method = "GET") {
    const query = new URLSearchParams(params).toString();
    const options = { method: method, headers: {} };

    const token = getToken();

    if (token) {
        options.headers["Authorization"] = "Bearer " + token;
    }

    let url = API_BASE_URL + path;

    if (method === "POST") {
        options.body = query;
        options.headers["Content-Type"] = "application/x-www-form-urlencoded";
    }
    else if (query) {
        url += "?" + query;
    }

    try {
        const response = await fetch(url, options);
        return await response.json();
    } catch (error) {
        console.error("Backend error:", error);
        return null;
    }
}

function apiPost(path, params = {}) {
    return api(path, params, "POST");
}

// Stops text from being treated as HTML
function escapeHtml(text) {
    return String(text)
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;");
}

function byId(id) {
    return document.getElementById(id);
}

async function testCppBackend() {
    const data = await api("/api/health");

    if (data) {
        console.log("C++ Backend Connected!", data);
    } else {
        console.error("C++ Backend is not running on " + API_BASE_URL);
    }
}

// =====================================================
// Dark Light Mode
// =====================================================

const themeBtn = document.getElementById("theme-toggle");

if (themeBtn) {

    themeBtn.addEventListener("click", () => {

        document.body.classList.toggle("light-mode");

        if (document.body.classList.contains("light-mode")) {
            themeBtn.innerHTML = "☀️";
        }
        else {
            themeBtn.innerHTML = "🌙";
        }

    });

}

// =====================================================
// Scroll Animation
// =====================================================

const sections = document.querySelectorAll("section");

sections.forEach(section => {
    section.classList.add("animate");
});

function revealSections() {

    sections.forEach(section => {

        let position = section.getBoundingClientRect().top;

        if (position < window.innerHeight - 100) {
            section.classList.add("show");
        }

    });

}

window.addEventListener("scroll", revealSections);
window.addEventListener("resize", revealSections);

// Without this, sections already on screen stay invisible until the
// visitor scrolls (short pages never scroll at all)
revealSections();
window.addEventListener("load", revealSections);

// =====================================================
// Counter Animation
// =====================================================

const counters = document.querySelectorAll(".counter");

counters.forEach(counter => {

    counter.innerHTML = "0";

    const updateCounter = () => {

        const target = +counter.getAttribute("data-target");

        const current = +counter.innerHTML;

        const increment = target / 100;

        if (current < target) {

            counter.innerHTML = Math.ceil(current + increment);

            setTimeout(updateCounter, 20);

        }
        else {

            counter.innerHTML = target + "+";

        }

    };

    updateCounter();

});

// =====================================================
// Contact form
// =====================================================

const contactForm = document.querySelector("#contact .contact-form");

if (contactForm) {

    // "submit" respects the required fields and stops the page reload
    contactForm.addEventListener("submit", (e) => {

        e.preventDefault();

        alert("Thank you for contacting EmpowerTech! We will reply soon.");

        contactForm.reset();

    });

}

// =====================================================
// COURSE SEARCH SYSTEM
// The C++ backend (BST search) is asked first. If it does not know
// the course, or is not running, the old keyword search is used.
// =====================================================

// Backend course name -> your page
const COURSE_PAGES = {
    "ai fundamentals": "ai.html",
    "machine learning": "ai.html",
    "cloud computing": "cloud.html",
    "cybersecurity": "cybersecurity.html",
    "data science": "datascience.html",
    "programming": "programming.html",
    "web development": "web-development.html"
};

// Your original keyword search (unchanged logic)
function keywordSearchRedirect(search) {

    if (search.includes("web") ||
        search.includes("html") ||
        search.includes("css") ||
        search.includes("javascript")) {

        window.location.href = "web-development.html";
    }

    else if (search.includes("programming") ||
             search.includes("python") ||
             search.includes("c++") ||
             search.includes("java")) {

        window.location.href = "programming.html";
    }

    else if (/\bai\b/.test(search) ||
             search.includes("artificial intelligence") ||
             search.includes("machine learning")) {

        window.location.href = "ai.html";
    }

    else if (search.includes("cyber") ||
             search.includes("security")) {

        window.location.href = "cybersecurity.html";
    }

    else if (search.includes("cloud") ||
             search.includes("aws")) {

        window.location.href = "cloud.html";
    }

    else if (search.includes("data") ||
             search.includes("sql")) {

        window.location.href = "datascience.html";
    }

    else {

        alert("Course not found. Try Web Development, AI, Cloud, Programming.");

    }
}

document.addEventListener("DOMContentLoaded", function () {

    const searchInput = document.getElementById("searchInput");

    if (searchInput) {

        searchInput.addEventListener("keypress", async function (event) {

            if (event.key !== "Enter") {
                return;
            }

            let search = searchInput.value.toLowerCase().trim();

            if (search === "") {
                return;
            }

            // 1) Ask the C++ backend (exact course name)
            const result = await api("/api/resources/search", { query: search });

            if (result && result.found) {

                const page = COURSE_PAGES[result.course.toLowerCase()];

                if (page) {
                    window.location.href = page;
                    return;
                }
            }

            // 2) Fallback: keyword search
            keywordSearchRedirect(search);

        });

    }

});

// =====================================================
// CHATBOT - open / close window
// =====================================================

const chatBtn = document.getElementById("chat-btn");
const chatWindow = document.getElementById("chat-window");

if (chatBtn && chatWindow) {

    chatBtn.addEventListener("click", () => {

        if (chatWindow.style.display === "block") {
            chatWindow.style.display = "none";
        }
        else {
            chatWindow.style.display = "block";
        }

    });

}

// =====================================================
// EMPOWERBOT
// Scholarship / internship / recommendation questions use live
// data from the C++ backend. Everything else uses the fixed replies.
// =====================================================

const sendBtn = document.getElementById("send-btn");
const userMessage = document.getElementById("userMessage");
const chatBody = document.getElementById("chat-body");

// Builds a short text list from /api/opportunities/search
async function opportunityReply(keyword, intro, fallback) {

    const data = await api("/api/opportunities/search", {
        query: keyword,
        field: "category",
        sort: "deadline"
    });

    if (!data || data.error || data.count === 0) {
        return fallback;
    }

    const top = data.opportunities.slice(0, 3).map(o =>
        `${escapeHtml(o.title)} (${escapeHtml(o.organization)}, deadline ${escapeHtml(o.deadline)})`
    ).join("<br>• ");

    return intro + "<br>• " + top;
}

// true if the message contains any of the words as a whole word
function has(message, ...words) {
    return words.some(w => new RegExp("\\b" + w + "\\b").test(message));
}

async function getBotReply(message) {

    if (message.includes("scholarship")) {
        return await opportunityReply(
            "scholarship",
            "Upcoming scholarships (nearest deadline first):",
            "You can explore scholarships in the Opportunities section. Build skills, maintain a good profile and apply regularly."
        );
    }

    if (message.includes("internship")) {
        return await opportunityReply(
            "internship",
            "Upcoming internships (nearest deadline first):",
            "For internships, improve your resume, create projects, maintain GitHub and apply on career platforms."
        );
    }

    if (message.includes("hackathon")) {
        return await opportunityReply(
            "hackathon",
            "Upcoming hackathons (nearest deadline first):",
            "Check the Opportunities section for hackathons."
        );
    }

    if (message.includes("recommend") || message.includes("best course")) {

        const data = await api("/api/recommendations");

        if (data && data.recommendations) {
            const top = data.recommendations.slice(0, 3)
                .map(r => escapeHtml(r.name)).join(", ");
            return "Our top recommended courses right now: " + top + ".";
        }
    }

    if (message.includes("web") || message.includes("html")) {
        return "You can learn Web Development from our Learning Hub. Start with HTML, CSS, JavaScript, then move to React and Node.js.";
    }

    if (message.includes("python") || message.includes("programming")) {
        return "For programming, practice C++, Python, Java and solve coding problems on platforms like LeetCode and Codeforces.";
    }

    if (has(message, "ai") || message.includes("machine learning")) {
        return "Artificial Intelligence includes Machine Learning, Deep Learning and Generative AI. Start with Python and mathematics basics.";
    }

    if (message.includes("resume") || has(message, "cv")) {
        return "A good resume should include skills, projects, education, achievements and links to your portfolio.";
    }

    if (message.includes("career") || message.includes("job")) {
        return "Visit Career Center for resume tips, interview preparation, jobs and freelancing guidance.";
    }

    if (has(message, "hello", "hi", "hey")) {
        return "Hello! 👋 I am EmpowerBot. I can help you with courses, careers, scholarships and technology.";
    }

    return "I can help you with Web Development, Programming, AI, Cyber Security, Cloud, Data Science, Scholarships and Careers.";
}

async function sendChatMessage() {

    const original = userMessage.value.trim();

    if (original === "") {
        return;
    }

    const message = original.toLowerCase();

    chatBody.innerHTML += `<p><b>You:</b> ${escapeHtml(original)}</p>`;

    userMessage.value = "";

    const reply = await getBotReply(message);

    chatBody.innerHTML += `<p><b>EmpowerBot:</b> ${reply}</p>`;

    chatBody.scrollTop = chatBody.scrollHeight;
}

if (sendBtn && userMessage && chatBody) {

    sendBtn.addEventListener("click", sendChatMessage);

    // Pressing Enter also sends
    userMessage.addEventListener("keypress", (event) => {
        if (event.key === "Enter") {
            sendChatMessage();
        }
    });

}

// =====================================================
// BACKEND PAGES
// Each block runs only if its element exists on the page,
// so one script.js can be used by every HTML file.
// =====================================================

// -----------------------------------------------------
// A) OPPORTUNITIES HUB   (opportunities.html)
//   #opportunity-list      container for the cards
//   #opportunity-sort      <select> deadline|stipend|date|category|title
//   #opportunity-search    <input> keyword (optional)
//   #opportunity-category  <select> ""|Internship|Scholarship|Hackathon|Competition|Job
//   #show-saved            <button> shows saved items (optional)
// -----------------------------------------------------

async function loadOpportunities(savedOnly = false) {

    const list = byId("opportunity-list");

    if (!list) {
        return;
    }

    const sort = byId("opportunity-sort") ? byId("opportunity-sort").value : "deadline";
    const keyword = byId("opportunity-search") ? byId("opportunity-search").value.trim() : "";
    // A page like scholarships.html can fix the category with
    // <div id="opportunity-list" data-category="Scholarship">
    const category = byId("opportunity-category")
        ? byId("opportunity-category").value
        : (list.dataset.category || "");

    let data;

    if (savedOnly) {
        data = await api("/api/opportunities/saved");
    }
    else if (keyword) {
        data = await api("/api/opportunities/search", { query: keyword, field: "all", sort: sort });
    }
    else {
        data = await api("/api/opportunities/filter", { category: category, sort: sort });
    }

    if (!data) {
        list.innerHTML = "<p>Backend is not running. Start the C++ server.</p>";
        return;
    }

    if (data.error) {
        list.innerHTML = `<p>${escapeHtml(data.error)}</p>`;
        return;
    }

    let items = data.opportunities;

    // search / saved list + category: the backend returns all categories,
    // so filter here
    if ((keyword || savedOnly) && category) {
        items = items.filter(o => o.type.toLowerCase() === category.toLowerCase());
    }

    if (items.length === 0) {
        list.innerHTML = "<p>No opportunities found.</p>";
        return;
    }

    list.innerHTML = items.map(o => `
        <div class="card">
            <h3>${escapeHtml(o.title)}</h3>
            <p>${escapeHtml(o.type)} · ${escapeHtml(o.organization)}</p>
            <p>Deadline: ${escapeHtml(o.deadline)}</p>
            <p>${o.amount > 0 ? "PKR " + o.amount.toLocaleString() : "No stipend / prize"}</p>
            <button class="btn" data-save-id="${o.id}" data-saved="${o.saved}">
                ${o.saved ? "Unsave" : "Save"}
            </button>
        </div>
    `).join("");
}

function initOpportunities() {

    const list = byId("opportunity-list");

    if (!list) {
        return;
    }

    let showingSaved = false;

    ["opportunity-sort", "opportunity-category"].forEach(id => {
        if (byId(id)) {
            byId(id).addEventListener("change", () => {
                showingSaved = false;
                loadOpportunities();
            });
        }
    });

    if (byId("opportunity-search")) {
        byId("opportunity-search").addEventListener("input", () => {
            showingSaved = false;
            loadOpportunities();
        });
    }

    if (byId("show-saved")) {
        byId("show-saved").addEventListener("click", () => {
            showingSaved = !showingSaved;
            loadOpportunities(showingSaved);
        });
    }

    // Save / Unsave buttons (event delegation)
    list.addEventListener("click", async (event) => {

        const button = event.target.closest("[data-save-id]");

        if (!button) {
            return;
        }

        const saved = button.dataset.saved === "true";

        await api(saved ? "/api/opportunities/unsave" : "/api/opportunities/save",
                  { id: button.dataset.saveId });

        loadOpportunities(showingSaved);
    });

    loadOpportunities();
}

// -----------------------------------------------------
// B) LEARNING PATH   (BFS / DFS)
//   #learning-field        <select> Web Development | AI/ML | Data Science |
//                                   Cybersecurity | Cloud Computing | Programming
//   #learning-mode         <select> bfs | dfs   (optional, default bfs)
//   #learning-path-btn     <button>
//   #learning-path-output  where the path is shown
// -----------------------------------------------------

function initLearningPath() {

    const button = byId("learning-path-btn");
    const output = byId("learning-path-output");

    if (!button || !output) {
        return;
    }

    button.addEventListener("click", async () => {

        const field = byId("learning-field") ? byId("learning-field").value : "AI/ML";
        const mode = byId("learning-mode") ? byId("learning-mode").value : "bfs";

        const data = await api("/api/learning-path", { field: field, mode: mode });

        if (!data) {
            output.innerHTML = "Backend is not running. Start the C++ server.";
            return;
        }

        if (data.error) {
            output.textContent = data.error;
            return;
        }

        output.innerHTML =
            `<p><b>${escapeHtml(data.algorithm)}</b> path for ${escapeHtml(data.field)}:</p>` +
            `<p>${data.path.map(escapeHtml).join(" → ")}</p>`;
    });
}

// -----------------------------------------------------
// C) LEARNING HUB COURSES   (linked list + BST)
//   #course-list   container; shows all courses, alphabetical
// -----------------------------------------------------

async function loadCourses() {

    const list = byId("course-list");

    if (!list) {
        return;
    }

    const data = await api("/api/courses/sorted");

    if (!data) {
        list.innerHTML = "<p>Backend is not running. Start the C++ server.</p>";
        return;
    }

    list.innerHTML = data.courses.map(c => `
        <div class="card">
            <h3>${escapeHtml(c.name)}</h3>
            <p>${escapeHtml(c.category)} · Score ${c.score}</p>
            <button class="btn" data-complete="${escapeHtml(c.name)}" ${c.completed ? "disabled" : ""}>
                ${c.completed ? "Completed ✔" : "Mark completed"}
            </button>
        </div>
    `).join("");
}

function initCourses() {

    const list = byId("course-list");

    if (!list) {
        return;
    }

    list.addEventListener("click", async (event) => {

        const button = event.target.closest("[data-complete]");

        if (!button) {
            return;
        }

        await api("/api/courses/complete", { name: button.dataset.complete });

        loadCourses();
    });

    loadCourses();
}

// -----------------------------------------------------
// D) CAREER CENTER
//   Interview questions:
//     #interview-list   container
//     #interview-type   <select> all | technical | hr   (optional)
//     #interview-query  <input> keyword                 (optional)
//   Career resources:
//     #career-resources-list   container
//   Resume (form with id="resume-form"), inputs with these ids:
//     resume-name, resume-email, resume-phone, resume-education,
//     resume-skills, resume-experience, resume-projects
//   Optional #resume-status to show "Saved".
// -----------------------------------------------------

async function loadInterviewQuestions() {

    const list = byId("interview-list");

    if (!list) {
        return;
    }

    const type = byId("interview-type") ? byId("interview-type").value : "all";
    const query = byId("interview-query") ? byId("interview-query").value : "";

    const data = await api("/api/career/interview", { type: type, query: query });

    if (!data) {
        list.innerHTML = "<p>Backend is not running. Start the C++ server.</p>";
        return;
    }

    if (data.error) {
        list.innerHTML = `<p>${escapeHtml(data.error)}</p>`;
        return;
    }

    if (data.count === 0) {
        list.innerHTML = "<p>No questions found.</p>";
        return;
    }

    list.innerHTML = data.questions.map(q => `
        <details class="card">
            <summary>[${escapeHtml(q.type)}] ${escapeHtml(q.question)}</summary>
            <p>${escapeHtml(q.answer)}</p>
        </details>
    `).join("");
}

function initInterview() {

    if (!byId("interview-list")) {
        return;
    }

    if (byId("interview-type")) {
        byId("interview-type").addEventListener("change", loadInterviewQuestions);
    }

    if (byId("interview-query")) {
        byId("interview-query").addEventListener("input", loadInterviewQuestions);
    }

    loadInterviewQuestions();
}

async function loadCareerResources() {

    const list = byId("career-resources-list");

    if (!list) {
        return;
    }

    const data = await api("/api/career/resources");

    if (!data) {
        list.innerHTML = "<p>Backend is not running. Start the C++ server.</p>";
        return;
    }

    list.innerHTML = data.resources.map(r => `
        <div class="card">
            <h3>${escapeHtml(r.title)}</h3>
            <p>${escapeHtml(r.category)}</p>
            <p>${escapeHtml(r.description)}</p>
            <a href="${escapeHtml(r.url)}"
               ${r.url.startsWith("http") ? 'target="_blank" rel="noopener"' : ""}>Open</a>
        </div>
    `).join("");
}

const RESUME_FIELDS = ["name", "email", "phone", "education",
                       "skills", "experience", "projects"];

async function initResume() {

    const form = byId("resume-form");

    if (!form) {
        return;
    }

    // Fill the form with the saved resume
    const data = await api("/api/career/resume");

    if (data && data.resume) {
        RESUME_FIELDS.forEach(key => {
            const input = byId("resume-" + key);
            if (input) {
                input.value = data.resume[key] || "";
            }
        });
    }

    // Save the resume
    form.addEventListener("submit", async (event) => {

        event.preventDefault();

        const params = {};

        RESUME_FIELDS.forEach(key => {
            const input = byId("resume-" + key);
            if (input) {
                params[key] = input.value;
            }
        });

        const result = await api("/api/career/resume/update", params);

        const status = byId("resume-status");

        if (status) {
            status.textContent = (result && !result.error)
                ? "Resume saved ✔"
                : "Could not save. Is the C++ server running?";
        }
    });
}

// -----------------------------------------------------
// E) LIVE LEARNING PATH on the roadmap pages (ai.html, cloud.html ...)
//   <div id="live-path" data-field="AI/ML"></div>
//   data-field: Web Development | AI/ML | Data Science |
//               Cybersecurity | Cloud Computing | Programming
// -----------------------------------------------------

async function initLivePath() {

    const box = byId("live-path");

    if (!box) {
        return;
    }

    const data = await api("/api/learning-path", {
        field: box.dataset.field || "AI/ML",
        mode: "bfs"
    });

    if (!data) {
        box.innerHTML = "<p>Backend is not running. Start the C++ server.</p>";
        return;
    }

    if (data.error) {
        box.textContent = data.error;
        return;
    }

    box.innerHTML =
        `<p style="text-align:center;margin-bottom:20px;">
            Shortest route (${escapeHtml(data.algorithm)}) from
            <b>${escapeHtml(data.start)}</b> to <b>${escapeHtml(data.destination)}</b>
         </p>
         <div class="tech-stack" style="margin-top:0;">` +
        data.path.map(step => `<span>${escapeHtml(step)}</span>`)
                 .join('<b style="color:#38bdf8;align-self:center;">→</b>') +
        `</div>`;
}

// -----------------------------------------------------
// F) STUDENT ACCOUNT  (login.html, dashboard.html, nav link)
// -----------------------------------------------------

// Nav link: "Login" when logged out, "Dashboard" when logged in
async function initAccountLink() {

    const link = byId("account-link");

    if (!link) {
        return;
    }

    if (!getToken()) {
        link.textContent = "Login";
        link.href = "login.html";
        return;
    }

    link.textContent = "Dashboard";
    link.href = "dashboard.html";

    // Check that the token is still valid (it expires after 7 days)
    const me = await api("/api/student/me");

    if (me && me.error) {
        setToken("");
        link.textContent = "Login";
        link.href = "login.html";
    }
}

function showAuthMessage(text, isError) {

    const box = byId("auth-message");

    if (box) {
        box.textContent = text;
        box.className = isError ? "auth-error" : "auth-ok";
    }
}

function initLoginPage() {

    const loginForm = byId("login-form");
    const registerForm = byId("register-form");

    if (!loginForm && !registerForm) {
        return;
    }

    // Already logged in? Go straight to the dashboard
    if (getToken()) {
        api("/api/student/me").then(me => {
            if (me && !me.error) {
                window.location.href = "dashboard.html";
            }
        });
    }

    async function submit(path, form, button, busyText) {

        const data = {};

        new FormData(form).forEach((value, key) => { data[key] = value; });

        const label = button.textContent;

        button.disabled = true;
        button.textContent = busyText;

        const result = await apiPost(path, data);

        button.disabled = false;
        button.textContent = label;

        if (!result) {
            showAuthMessage("Backend is not running. Start the C++ server.", true);
            return;
        }

        if (result.error) {
            showAuthMessage(result.error, true);
            return;
        }

        setToken(result.token);

        showAuthMessage("Welcome, " + result.student.name + "! Opening your dashboard...", false);

        setTimeout(() => { window.location.href = "dashboard.html"; }, 600);
    }

    if (loginForm) {
        loginForm.addEventListener("submit", (event) => {
            event.preventDefault();
            submit("/api/student/login", loginForm,
                   loginForm.querySelector("button"), "Logging in...");
        });
    }

    if (registerForm) {
        registerForm.addEventListener("submit", (event) => {
            event.preventDefault();
            submit("/api/student/register", registerForm,
                   registerForm.querySelector("button"), "Creating account...");
        });
    }
}

const STATUS_LABELS = { none: "Not started", started: "In progress", completed: "Completed ✔" };

let profileFormFilled = false;

async function renderDashboard() {

    const data = await api("/api/student/dashboard");

    if (!data) {
        byId("dash-sub").textContent = "Backend is not running. Start the C++ server.";
        return;
    }

    if (data.error) {
        // token missing or expired
        setToken("");
        window.location.href = "login.html";
        return;
    }

    byId("dash-welcome").textContent = "👤 Welcome, " + data.student.name;
    byId("dash-sub").textContent =
        "Student ID: " + data.student.id + "  ·  " + data.student.email +
        "  ·  Member since " + data.student.joined;

    const st = data.stats;

    byId("dash-stats").innerHTML = `
        <div class="impact-box"><h2>${st.completed}</h2><p>Completed</p></div>
        <div class="impact-box"><h2>${st.started}</h2><p>In progress</p></div>
        <div class="impact-box"><h2>${st.totalCourses}</h2><p>Total courses</p></div>
        <div class="impact-box"><h2>${st.percent}%</h2><p>Overall progress</p></div>`;

    byId("dash-bar").style.width = st.percent + "%";

    const favorites = new Set(data.favorites.map(f => f.type + "|" + f.item));

    byId("dash-courses").innerHTML = data.courses.map(c => {

        const isFav = favorites.has("course|" + c.name);

        return `
        <div class="card">
            <h3>${escapeHtml(c.name)}</h3>
            <p>${escapeHtml(c.category)} · <b>${STATUS_LABELS[c.status] || c.status}</b></p>
            <div class="card-actions">
                ${c.status !== "started" && c.status !== "completed" ?
                    `<button class="btn" data-course="${escapeHtml(c.name)}" data-status="started">Start</button>` : ""}
                ${c.status !== "completed" ?
                    `<button class="btn" data-course="${escapeHtml(c.name)}" data-status="completed">Mark completed</button>` : ""}
                ${c.status !== "none" ?
                    `<button class="btn btn-light" data-course="${escapeHtml(c.name)}" data-status="none">Reset</button>` : ""}
                <button class="btn btn-light" data-fav-type="course"
                        data-fav-item="${escapeHtml(c.name)}" data-fav-add="${!isFav}">
                    ${isFav ? "♥ Favorite" : "♡ Favorite"}
                </button>
            </div>
        </div>`;
    }).join("");

    byId("dash-recommended").innerHTML = data.recommended.length === 0
        ? "<p>You finished everything. Great work!</p>"
        : data.recommended.map(r => `
            <div class="card">
                <h3>${escapeHtml(r.name)}</h3>
                <p>${escapeHtml(r.category)} · Score ${r.score}</p>
            </div>`).join("");

    byId("dash-favorites").innerHTML = data.favorites.length === 0
        ? "<p>No favorites yet. Press ♡ Favorite on a course.</p>"
        : "<ul>" + data.favorites.map(f => `
            <li>${escapeHtml(f.type)}: <b>${escapeHtml(f.item)}</b>
                <button class="btn btn-light" data-fav-type="${escapeHtml(f.type)}"
                        data-fav-item="${escapeHtml(f.item)}" data-fav-add="false">Remove</button>
            </li>`).join("") + "</ul>";

    // Fill the profile form only once, so typing is never overwritten
    if (!profileFormFilled) {

        const form = byId("profile-form");

        form.elements["name"].value       = data.student.name;
        form.elements["university"].value = data.profile.university;
        form.elements["degree"].value     = data.profile.degree;
        form.elements["interests"].value  = data.profile.interests;
        form.elements["bio"].value        = data.profile.bio;

        profileFormFilled = true;
    }
}

function initDashboard() {

    const root = byId("dashboard");

    if (!root) {
        return;
    }

    if (!getToken()) {
        window.location.href = "login.html";
        return;
    }

    renderDashboard();

    // Course status buttons and favorite buttons (event delegation)
    root.addEventListener("click", async (event) => {

        const statusButton = event.target.closest("[data-course]");

        if (statusButton) {
            await apiPost("/api/student/progress/update", {
                course: statusButton.dataset.course,
                status: statusButton.dataset.status
            });
            renderDashboard();
            return;
        }

        const favButton = event.target.closest("[data-fav-item]");

        if (favButton) {
            const add = favButton.dataset.favAdd === "true";
            await apiPost(add ? "/api/student/favorites/add" : "/api/student/favorites/remove", {
                type: favButton.dataset.favType,
                item: favButton.dataset.favItem
            });
            renderDashboard();
        }
    });

    // Save profile
    byId("profile-form").addEventListener("submit", async (event) => {

        event.preventDefault();

        const data = {};

        new FormData(event.target).forEach((value, key) => { data[key] = value; });

        const result = await apiPost("/api/student/profile/update", data);

        byId("profile-status").textContent =
            (result && !result.error) ? "Profile saved ✔"
                                      : ((result && result.error) || "Could not save the profile");

        renderDashboard();
    });

    // Find a student by ID (hash table lookup in the backend)
    byId("lookup-form").addEventListener("submit", async (event) => {

        event.preventDefault();

        const id = byId("lookup-id").value.trim();

        if (!id) {
            return;
        }

        const result = await api("/api/student/lookup", { id: id });

        byId("lookup-result").textContent =
            !result ? "Backend is not running."
            : result.found ? "Found: " + result.name + " (" + result.id + ")"
            : "No student with that ID.";
    });

    // Log out
    byId("logout-btn").addEventListener("click", async () => {
        await apiPost("/api/student/logout");
        setToken("");
        window.location.href = "login.html";
    });
}

// =====================================================
// START EVERYTHING WHEN THE PAGE IS READY
// =====================================================

document.addEventListener("DOMContentLoaded", function () {

    testCppBackend();

    initOpportunities();
    initLearningPath();
    initCourses();
    initInterview();
    loadCareerResources();
    initResume();
    initLivePath();

    initAccountLink();
    initLoginPage();
    initDashboard();

});
