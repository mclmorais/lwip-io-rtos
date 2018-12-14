var interval;
window.onload = function ()
{
  document.getElementById('about').onclick = loadAbout;
  document.getElementById('io_http').onclick = loadIOHttp;

  loadIOHttp();
  speedGet();

}

function loadAbout()
{
  loadPage("about.htm");
  return false;
}

function loadOverview()
{
  loadPage("overview.htm");
  return false;
}

function loadBlock()
{
  loadPage("block.htm");
  return false;
}

function loadIOHttp()
{
  loadPage("io_http.htm");
  ledstateGet();
  return false;
}

function SetFormDefaults()
{
  document.iocontrol.LEDOn.checked = ls;
  document.iocontrol.speed_percent.value = sp;
}

function toggle_led()
{
  var req = false;

  function ToggleComplete()
  {
    if (req.readyState == 4)
    {
      if (req.status == 200)
      {
        document.getElementById("ledstate").innerHTML = "<div>" +
          req.responseText + "</div>";
      }
    }
  }

  if (window.XMLHttpRequest)
  {
    req = new XMLHttpRequest();
  }
  else if (window.ActiveXObject)
  {
    req = new ActiveXObject("Microsoft.XMLHTTP");
  }
  if (req)
  {
    req.open("GET", "/cgi-bin/toggle_led", true);
    req.onreadystatechange = ToggleComplete;
    req.send(null);
  }
}

function speedGetInterval()
{
  if (interval)
    return;
  interval = setInterval(speedGet, 500)
}

function speedRangeSet()
{
  speedSet(document.getElementById("reference_range").value);

}


function speedSet(value)
{
  var req = false;

  function speedComplete()
  {
    if (req.readyState == 4)
    {
      if (req.status == 200)
      {
        let number = req.responseText;
        if (number < 10) number = "00" + number;
        else if (number < 100) number = "0" + number;

        document.getElementById("current_speed").innerHTML = number;
      }
    }
  }

  if (window.XMLHttpRequest)
  {
    req = new XMLHttpRequest();
  }

  else if (window.ActiveXObject)
  {
    req = new ActiveXObject("Microsoft.XMLHTTP");
  }

  if (req)
  {
    req.open("GET", "/cgi-bin/set_speed?percent=" + value +
      "&id" + Math.round(Math.random() * 1000), true);
    req.onreadystatechange = speedComplete;
    req.send(null);
  }
}

function ledstateGet()
{
  var led = false;
  function ledComplete()
  {
    if (led.readyState == 4)
    {
      if (led.status == 200)
      {
        document.getElementById("ledstate").innerHTML = "<div>" +
          led.responseText + "</div>";
      }
    }
  }

  if (window.XMLHttpRequest)
  {
    led = new XMLHttpRequest();
  }
  else if (window.ActiveXObject)
  {
    led = new ActiveXObject("Microsoft.XMLHTTP");
  }
  if (led)
  {
    led.open("GET", "/ledstate?id=" + Math.round(Math.random() * 1000), true);
    led.onreadystatechange = ledComplete;
    led.send(null);
  }
}

function speedGet()
{

  var req = false;
  function speedReturned()
  {
    if (req.readyState == 4)
    {
      if (req.status == 200)
      {
        document.getElementById("current_speed").innerHTML = req.responseText;
        document.getElementById("reference_range").value = req.responseText;
      }
    }
  }

  if (window.XMLHttpRequest)
  {
    req = new XMLHttpRequest();
  }
  else if (window.ActiveXObject)
  {
    req = new ActiveXObject("Microsoft.XMLHTTP");
  }
  if (req)
  {
    req.open("GET", "/get_speed?id=" + Math.round(Math.random() * 1000), true);
    req.onreadystatechange = speedReturned;
    req.send(null);
  }
}

function loadPage(page)
{
  if (window.XMLHttpRequest)
  {
    xmlhttp = new XMLHttpRequest();
  }
  else
  {
    xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
  }

  xmlhttp.open("GET", page, true);
  xmlhttp.setRequestHeader("Content-type",
    "application/x-www-form-urlencoded");
  xmlhttp.send();

  xmlhttp.onreadystatechange = function ()
  {
    if ((xmlhttp.readyState == 4) && (xmlhttp.status == 200))
    {
      document.getElementById("content").innerHTML = xmlhttp.responseText;
    }
  }
}
