/*
 *  https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectSparkle
 */

void sequence_BLACK() {
  for ( int i=0; i<pref_num_leds; i++ ) {
    leds[i] = CRGB( 0, 0, 0 );
  }
}

void sequence_WIFI_SmartConfig() {
  // All LED's are RED
  // Every 2nd LED is bright
  for ( int i=0; i<pref_num_leds; i=i+3 ) {
    leds[i]   = CRGB( 255,0,0);
    leds[i+1] = CRGB( 0,255,0);
    leds[i+2] = CRGB( 0,0,255);    
  }
}

void sequence_WIFI_Connecting() {
  // All LED's are RED
  // Every 2nd LED is bright
  for ( int i=0; i<pref_num_leds; i++ ) {
    leds[i] = CRGB( 0, 50 + (i%2==0?200:0 ) , 0 );
  }
}

void sequence_ARTNET_Starting() {
  // All LED's are BLUE
  // Every 2nd LED is bright
  for ( int i=0; i<pref_num_leds; i++ ) {
    leds[i] = CRGB( 50 + (i%2==0?200:0 ) , 0, 0 );
  }
}

void sequence_WebServer_Starting() {
  // All LED's are GREEN
  // Every 2nd LED is bright
  for ( int i=0; i<pref_num_leds; i++ ) {
    leds[i] = CRGB( 0, 0, 50 + (i%2==0?200:0 ));
  }
}

void sequence_config_mode(){
  // Everything black, except start and end LED's
  for ( int i=0; i<pref_num_leds; i++ ) {
    leds[i] = CRGB( 0,0,0 );
    if ( i == pref_led_first or i == pref_led_last) {
      leds[i] = CRGB( 255,255,255 );
    }
  }     
}


void sequence_startup(){

    // Startup Sequence 
    for ( int i=0; i<(pref_num_leds + 6); i++ ) {
        //Green
        if ( i < pref_num_leds ) 
            leds[i] = CRGB( 20 ,20, 0);

        //Red
        if ( i-1 >= 0 && i-1 < pref_num_leds ) {
            leds[i-1] = CRGB( 0,255,0);
        }

        //Green
        if ( i-2 >= 0 && i-2 < pref_num_leds) {
            leds[i-2] = CRGB( 255,0,0);
        }

        if ( i-3 >= 0 && i-3 < pref_num_leds) {
            //Blue
            leds[i-3] = CRGB( 0,0,255); 
        }

        if ( i-4 >= 0 && i-4 < pref_num_leds) {
            //Blue
            leds[i-4] = CRGB( 0,0,50); 
        }

            if ( i-5 >= 0 && i-5 < pref_num_leds) {
            // Off
            leds[i-5] = CRGB( 0,0,0); 
        }   

        FastLED.show();
        delay(15);

    }
}



void sequende_RGB( int R, int G, int B, int StrobeSpeedA, int StrobeSpeedB ){
  // RGB Mode

  if ( StrobeSpeedA==0 && StrobeSpeedB==0 ) {
    startIndex = 1;
    last_chase_millis = millis();
  } else {
    // Map 0-255 as BPM... into MS
    unsigned int RGB_Millis   = 60000/(StrobeSpeedA+(StrobeSpeedB*5)); //map( StrobeSpeedA, 1, 255, 1000, 1 );

    if ( millis() >= last_chase_millis ) {
      startIndex ++;
      last_chase_millis = millis() + RGB_Millis;      
    }
  }
  
  // Paint RGB or Black, depending on if startindex is odd or even
  if ( startIndex%2 == 0 ) {
    for ( int i=0; i<pref_num_leds; i++ ) {
      leds[i] = CRGB( 0,0,0 );
    }    
  } else {
    for ( int i=0; i<pref_num_leds; i++ ) {
      leds[i] = CRGB( R, G, B );
    }
  }  

}

void sequence_pallet( int palette, int speed ){

  unsigned int increment;

  // Below 126
  if ( speed < 126 ) {
    millis_per_chase = map( speed, 0, 126, 3, 150 );
    increment = +1;
  // Stationary
  } else if (( speed >= 126 ) && ( speed <= 130 )) {
    millis_per_chase = 0;
    increment = 0;
  // Above 130
  } else if ( speed > 130 ) {
    millis_per_chase = map( speed, 131, 255, 150, 3 );
    increment = -1;
  }



  if( palette <  23)      { currentPalette = RainbowColors_p;         currentBlending = LINEARBLEND; }
  else if( palette < 46)  { currentPalette = RainbowStripeColors_p;   currentBlending = NOBLEND;  }
  else if( palette < 69)  { currentPalette = RainbowStripeColors_p;   currentBlending = LINEARBLEND; }
  else if( palette < 92)  { SetupPurpleAndGreenPalette();             currentBlending = LINEARBLEND; }
  else if( palette < 115) { SetupTotallyRandomPalette();              currentBlending = LINEARBLEND; }
  else if( palette < 138) { SetupBlackAndWhiteStripedPalette();       currentBlending = NOBLEND; }
  else if( palette < 161) { SetupBlackAndWhiteStripedPalette();       currentBlending = LINEARBLEND; }
  else if( palette < 184) { currentPalette = CloudColors_p;           currentBlending = LINEARBLEND; }
  else if( palette < 207) { currentPalette = PartyColors_p;           currentBlending = LINEARBLEND; }
  else if( palette < 230) { currentPalette = myRedWhiteBluePalette_p; currentBlending = NOBLEND;  }
  else { currentPalette = myRedWhiteBluePalette_p; currentBlending = LINEARBLEND; }

  // Perform Chase here and re-fill LED's
  if ( millis() > (last_chase_millis + millis_per_chase) ) {
    if ( increment != 0 ) {
      startIndex = startIndex + increment;      
      FillLEDsFromPaletteColors( startIndex );
    }
    last_chase_millis = millis();
  }
      
  
}

void sequence_glitter( int star_count, int fade, int R, int G, int B){
  // https://gist.github.com/mock-turtle/d59716ab96dca6c8ec0b
  
  //changing the third variable changes how quickly the lights fade
  fadeToBlackBy( leds, pref_num_leds, map( fade, 0, 255, 0, 100 ) );
  
  
  //changing this variable will increase the chance of a "star" popping up
  // addGlitter( star_count );
  if( random8() < star_count) {
    if ( R==0 && G==0 && B==0 ) {
      leds[ random16(pref_num_leds) ] += CRGB( random16(255), random16(255), random16(255) );
    } else {
      leds[ random16(pref_num_leds) ] += CRGB( B, R, G ); //CRGB::White;
    }
  }
}

byte heat[1024];

void sequence_fire(int Cooling, int Sparking, int SpeedDelay) {
  
  int cooldown;
 
  // Step 1.  Cool down every cell a little
  for( int i = 0; i < pref_num_leds; i++) {
    cooldown = random(0, ((Cooling * 10) / pref_num_leds) + 2);
   
    if(cooldown>heat[i]) {
      heat[i]=0;
    } else {
      heat[i]=heat[i]-cooldown;
    }
  }
 
  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for( int k= pref_num_leds - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
   
  // Step 3.  Randomly ignite new 'sparks' near the bottom
  if( random(255) < Sparking ) {
    int y = random(7);
    heat[y] = heat[y] + random(160,255);
    //heat[y] = random(160,255);
  }

  // Step 4.  Convert heat to LED colors
  for( int j = 0; j < pref_num_leds; j++) {
    setPixelHeatColor(j, heat[j] );
  }

  //showStrip();
  delay(SpeedDelay);
}
