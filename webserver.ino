void webserver_routine(void) {
  
  server.on("/", HTTP_GET, []() {
    Serial.printf("request root\n");
    server.sendHeader("Connection", "close");

     
    char serverIndex[2048];
    sprintf( serverIndex, 
            "<form method='POST' action='/save' enctype='multipart/form-data'>"
            "  Set Config mode for controller <input type='test' name='pref_config_mode' value='%d'><br>"
            
            "  Number of LED's in string <input type='test' name='pref_num_leds' value='%d'><br>"
            "  First LED Index <input type='test' name='pref_led_first' value='%d'><br>"
            "  Last LED Index<input type='test' name='pref_led_last' value='%d'><br>"

            "  ArtNET Universe <input type='test' name='pref_artnet_universe' value='%d'><br>"
            "  ArtNet Start Channel <input type='test' name='pref_artnet_startchannel' value='%d'><br>"
                        
            "  <input type='submit' value='Save'>"
            "</form>"
            
            "<form method='POST' action='/reset' enctype='multipart/form-data'>"
            "  <input type='submit' value='Reset'>"
            "</form>",
            pref_config_mode, 
            pref_num_leds, pref_led_first, pref_led_last,
            pref_artnet_universe, pref_artnet_startchannel );
            
    server.send(200, "text/html", serverIndex);
  });
  
  server.on("/save", HTTP_POST, []() {
    Serial.printf("save post\n");
    
    preferences.begin("my-app", false);
    for (uint8_t i = 0; i < server.args(); i++) {
      Serial.printf( "Set Preference NAME: %s VALUE: %d \n", server.argName(i).c_str(), server.arg(i).toInt() );
      preferences.putUInt(server.argName(i).c_str(), server.arg(i).toInt() );

      // Set value live without a reset
      if ( server.argName(i) == "pref_config_mode" ) {
        pref_config_mode = server.arg(i).toInt();
      } else if ( server.argName(i) == "pref_led_first" ) {
        pref_led_first = server.arg(i).toInt();
      } else if ( server.argName(i) == "pref_led_last" ) {
        pref_led_last = server.arg(i).toInt();
      } else if ( server.argName(i) == "pref_num_leds" ) {
        pref_num_leds = server.arg(i).toInt();
      } else if ( server.argName(i) == "pref_artnet_universe" ) {
        pref_artnet_universe = server.arg(i).toInt();
      } else if ( server.argName(i) == "pref_artnet_startchannel" ) {
        pref_artnet_startchannel = server.arg(i).toInt();
      }
      
    }
    preferences.end();
    
    //request->redirect("/");
    //server.send(302, "Found", "Location: /");
    server.sendHeader("Connection", "close");
  });

  server.on("/reset", HTTP_POST, []() {
    Serial.printf("reset post\n");
    ESP.restart();
  });
  
  server.begin();
}
