var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  // Construct URL
  var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' + pos.coords.latitude + '&lon=' + pos.coords.longitude + '&appid=2874bea34ea1f91820fa07af69939eea';
  
  console.log("Lat is " + pos.coords.latitude);
  console.log("Lon is " + pos.coords.longitude);
  console.log('URL is ' + url);

  // Send request to OpenWeatherMap
  xhrRequest(url, 'GET', 
    function(responseText) {
      console.log("Parsing JSON");
      
      var json = JSON.parse(responseText); // Parse JSON response
      console.log(JSON.parse(responseText));
			
			var temperature =  Math.round(((json.main.temp - 273.15) * 1.8) + 32);
			console.log("Temperature: " + temperature);
			
			var temperaturec =  Math.round(json.main.temp - 273.15);
			console.log("Temperature: " + temperaturec);
      
      // Assemble weather info into dictionary
      var dictionary = {
				"KEY_TEMPERATURE": temperature,
				"KEY_TEMPERATURE_IN_C": temperaturec,
      };

      // Send dictionary to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("Weather info sent to Pebble successfully!");
        },
        function(e) {
          console.log("Error sending weather info to Pebble!");
        }
      );
    }      
  );
}

function locationError(err) {
  console.log('Error requesting location!');
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS Ready!');

  getWeather(); // Get weather
});

Pebble.addEventListener('appmessage',
  function(e) {
    console.log('AppMessage received!');
		getWeather();
  }                
);

// ========== CONFIGURATION ========== //

Pebble.addEventListener('showConfiguration', function() {
  var url = 'http://659196e.ngrok.com';

  console.log('Showing configuration page: ' + url);

  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  var configData = JSON.parse(decodeURIComponent(e.response));

  console.log('Configuration page returned: ' + JSON.stringify(configData));

	// Send all keys to Pebble
	Pebble.sendAppMessage({
		useCelsius: configData.useCelsius ? 1 : 0
	}, function() {
		console.log('Send successful!');
	}, function() {
		console.log('Send failed!');
	});
});