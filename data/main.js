console.info("helloWorld");

function deferLoadNotify() {
  setTimeout(loadNotify, 300);
  console.info("defered");
  return true;
}
function loadNotify() {
  document.getElementById("notify_response").contentWindow.location.reload();
}
