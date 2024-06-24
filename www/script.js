const btn = document.querySelector("#btn");
const label = document.querySelector("#label");

let i = 0;

btn.onclick = () => {
    ++i;
    label.innerHTML = "Clicked " + i + " times!";
}