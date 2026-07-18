// Dark Light Mode

console.log("JavaScript Connected");

const themeBtn = document.getElementById("theme-toggle");


if(themeBtn){

    themeBtn.addEventListener("click",()=>{


        document.body.classList.toggle("light-mode");


        if(document.body.classList.contains("light-mode")){

            themeBtn.innerHTML = "☀️";

        }

        else{

            themeBtn.innerHTML = "🌙";

        }


    });

}
// Scroll Animation

const sections=document.querySelectorAll("section");


sections.forEach(section=>{

    section.classList.add("animate");

});


window.addEventListener("scroll",()=>{


    sections.forEach(section=>{


        let position=section.getBoundingClientRect().top;


        if(position < window.innerHeight - 100){

            section.classList.add("show");

        }


    });


});
// Counter Animation


const counters=document.querySelectorAll(".counter");


counters.forEach(counter=>{


counter.innerHTML="0";


const updateCounter=()=>{


const target=+counter.getAttribute("data-target");


const current=+counter.innerHTML;


const increment=target/100;



if(current < target){

counter.innerHTML=Math.ceil(current+increment);

setTimeout(updateCounter,20);

}

else{

counter.innerHTML=target+"+";

}


}


updateCounter();


});
const contactBtn=document.querySelector(".contact-form button");


if(contactBtn){

contactBtn.addEventListener("click",(e)=>{

alert("Thank you for contacting EmpowerTech! We will reply soon.");

});

}
// SEARCH FUNCTION
// =====================
// SEARCH FUNCTION
// =====================



// =======================
// COURSE SEARCH SYSTEM
// =======================

document.addEventListener("DOMContentLoaded", function () {

    const searchInput = document.getElementById("searchInput");

    console.log(searchInput); // should show your input element


    if (searchInput) {

        searchInput.addEventListener("keypress", function (event) {


            if (event.key === "Enter") {


                let search = searchInput.value.toLowerCase().trim();


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


                else if (search.includes("ai") ||
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


        });


    }


});


// CHATBOT

// =====================
// OPEN CHATBOT WINDOW
// =====================

const chatBtn = document.getElementById("chat-btn");
const chatWindow = document.getElementById("chat-window");


if(chatBtn && chatWindow){

    chatBtn.addEventListener("click",()=>{

        if(chatWindow.style.display === "block"){

            chatWindow.style.display = "none";

        }
        else{

            chatWindow.style.display = "block";

        }

    });

}
// =====================
// EMPOWERBOT
// =====================
console.log("Chatbot JS loaded");

console.log(document.getElementById("send-btn"));
console.log(document.getElementById("userMessage"));
console.log(document.getElementById("chat-body"));

const sendBtn = document.getElementById("send-btn");

const userMessage = document.getElementById("userMessage");

const chatBody = document.getElementById("chat-body");



if(sendBtn){


sendBtn.addEventListener("click",()=>{


let message = userMessage.value.toLowerCase().trim();


if(message==="")
return;



chatBody.innerHTML +=
`
<p><b>You:</b> ${message}</p>
`;



let reply="";



if(message.includes("web") || message.includes("html")){

reply="You can learn Web Development from our Learning Hub. Start with HTML, CSS, JavaScript, then move to React and Node.js.";

}


else if(message.includes("python") || message.includes("programming")){

reply="For programming, practice C++, Python, Java and solve coding problems on platforms like LeetCode and Codeforces.";

}


else if(message.includes("ai") || message.includes("machine learning")){

reply="Artificial Intelligence includes Machine Learning, Deep Learning and Generative AI. Start with Python and mathematics basics.";

}


else if(message.includes("scholarship")){

reply="You can explore scholarships in the Opportunities section. Build skills, maintain a good profile and apply regularly.";

}


else if(message.includes("internship")){

reply="For internships, improve your resume, create projects, maintain GitHub and apply on career platforms.";

}


else if(message.includes("resume") || message.includes("cv")){

reply="A good resume should include skills, projects, education, achievements and links to your portfolio.";

}


else if(message.includes("career") || message.includes("job")){

reply="Visit Career Center for resume tips, interview preparation, jobs and freelancing guidance.";

}


else if(message.includes("hello") || message.includes("hi")){

reply="Hello! 👋 I am EmpowerBot. I can help you with courses, careers, scholarships and technology.";

}


else{

reply="I can help you with Web Development, Programming, AI, Cyber Security, Cloud, Data Science, Scholarships and Careers.";

}




chatBody.innerHTML +=
`
<p><b>EmpowerBot:</b> ${reply}</p>
`;



userMessage.value="";


chatBody.scrollTop=chatBody.scrollHeight;



});


}
