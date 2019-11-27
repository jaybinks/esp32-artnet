void webserver_routine(void) {
  
  server.on("/", HTTP_GET, []() {
    
    last_http_millis = millis();
    
    server.sendHeader("Connection", "close");
     
    char serverIndex[5048];
    sprintf( serverIndex, 
            "<html>"
            " <head>"
            "   <link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css' integrity='sha384-ggOyR0iXCbMQv3Xipma34MD+dH/1fQ784/j6cY/iJTQUOhcWr7x9JvoRxT2MZw1T' crossorigin='anonymous'>"
            "   <script src='https://code.jquery.com/jquery-3.3.1.slim.min.js' integrity='sha384-q8i/X+965DzO0rT7abK41JStQIAqVgRVzpbzo5smXKp4YfRvH+8abtTE1Pi6jizo' crossorigin='anonymous'></script>"
            "   <script src='https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.14.7/umd/popper.min.js' integrity='sha384-UO2eT0CpHqdSJQ6hJty5KVphtPhzWj9WO1clHTMGa3JDZwrnQq4sF86dIHNDz0W1' crossorigin='anonymous'></script>"
            "   <script src='https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/js/bootstrap.min.js' integrity='sha384-JjSmVgyd0p3pXB1rRibZUAYoIIy6OrQ6VrjIEaFf/nJGzIxFDsf4x0xIM+B07jRM' crossorigin='anonymous'></script>"
            "   <meta name='viewport' content='width=device-width, initial-scale=1, shrink-to-fit=no'>"
            " </head>"
            "  <form method='POST' action='/save' enctype='multipart/form-data'>"
            "    <div class=form-group'><label class='col-md-4 control-label'>Set Config mode for controller </label><div class=col-md-4'>"

            "     <select  name='pref_mode' class='form-control input-md' onchange='this.form.submit()'>"
            "      <option value='2' %s>Test Mode</option>"
            "      <option value='1' %s>Config Mode</option>"
            "      <option value='0' %s>Show Mode</option>"
            "    </select> </div></div>"          

            "    <label class='col-md-4 control-label'>Number of LED's in string</label><div class='col-md-4'><input type='number' class='form-control input-md' name='pref_num_leds' value='%d' onchange='this.form.submit()'></div><br>"
            "    <label class='col-md-4 control-label'>First LED Index</label><div class='col-md-4'><input type='number' class='form-control input-md' name='pref_led_first' value='%d' onchange='this.form.submit()'></div><br>"
            "    <label class='col-md-4 control-label'>Last LED Index</label><div class='col-md-4'><input type='number' class='form-control input-md' name='pref_led_last' value='%d' onchange='this.form.submit()' ></div><br>"

            "    <label class='col-md-4 control-label'>ArtNET Universe</label><div class='col-md-4'><input type='number' class='form-control input-md' name='pref_universe' value='%d' onchange='this.form.submit()'></div><br>"
            "    <label class='col-md-4 control-label'>ArtNet Start Channel</label><div class='col-md-4'><input type='number' class='form-control input-md' name='pref_startchan' value='%d' onchange='this.form.submit()'></div><br>"
                        
            "    <input type='submit' class='btn btn-success'value='Save'>"
            "  </form>"
            
            "  <form method='POST' action='/reset' enctype='multipart/form-data'>"
            "    <input type='hidden' name='reset'>"
            "    <input type='submit' class='btn btn-danger' value='Reset'>"
            "  </form>"
            "</html>",
            
            (pref_config_mode==2)?"selected":"",
            (pref_config_mode==1)?"selected":"", 
            (pref_config_mode==0)?"selected":"", 

            pref_num_leds, pref_led_first, pref_led_last,
            pref_artnet_universe, pref_artnet_startchannel );
            
    server.send(200, "text/html", serverIndex);
  });
  
  server.on("/save", HTTP_POST, []() {
    last_http_millis = millis();

    preferences.begin("my-app", false);  
    for (uint8_t i = 0; i < server.args(); i++) {
      preferences.putUInt(server.argName(i).c_str(), server.arg(i).toInt() );
    }
    preferences.end();

    load_preferences();
    
    server.sendHeader("Location", "/",true); //Redirect to our html web page 
    server.send(302, "text/plane",""); 

    server.sendHeader("Connection", "close");
  });

  server.on("/reset", HTTP_POST, []() {
    char resetHTML[5048];
    sprintf( resetHTML, "<html><head><meta http-equiv='refresh' content='5; url=http://%s/'></head></html>" , ipaddress);
    
    server.send(200, "text/html", resetHTML);

    server.sendHeader("Connection", "close");
    Serial.printf("reset post\n");

    ESP.restart();
  });
  
  server.begin();
}
