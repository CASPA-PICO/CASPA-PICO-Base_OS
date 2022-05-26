<h1>CASPA-PICO Base OS</h1>
<p>Le programme est un projet <a href="https://platformio.org/">Platform IO</a> testé sur une ESP32 modèle : <a href="https://www.dfrobot.com/product-2231.html">DFR0654-F</a><br/></p>
<h2>Bibliothèques utilisées</h2>
<ul>
  <li><a href="https://github.com/CASPA-PICO/PLTP">PLTP</a></li>
  <li><a href="https://github.com/platformio/platform-espressif32">Espressif 32: development platform for PlatformIO</a></li>
  <li><a href="https://github.com/espressif/arduino-esp32">Arduino core for the ESP32</a></li>
  <li><a href="https://github.com/arkhipenko/Dictionary">Dictionary</a></li>
  <li><a href="https://github.com/JAndrassy/EthernetENC">EthernetENC</a></li>
  <li><a href="https://github.com/OPEnSLab-OSU/SSLClient">SSLClient</a></li>
  <li><a href="https://github.com/ThingPulse/esp8266-oled-ssd1306">ThingPulse OLED SSD1306 (ESP8266/ESP32/Mbed-OS)</a></li>
  <li><a href="https://github.com/me-no-dev/ESPAsyncWebServer">ESPAsyncWebServer</a></li>
  <li><a href="https://github.com/me-no-dev/AsyncTCP">AsyncTCP</a></li>
</ul>
<h2>Fonctionnement global du programme</h2>
<p>Lorsque le capteur est connecté à la base, les données sont transmise en Bluetooth du capteur vers le stockage local de la base.<br/>Une fois ce transfert terminé, les données sont transférés du stockage local de la base vers le serveur web</p>
<h2>Plusieurs tâches tournent en parallèles dans la base</h2>
<h3>Tâche Ethernet</h3>
<p>La tâche Ethernet gère le port Ethernet et sa connexion au réseau, l'obtention d'une adresse IP grâce à un DHCP</p>
<h3>Tâche Wifi</h3>
<p>La tâche Wifi gère le Wifi intégré au microcontrôleur, essaie de se connecter au dernier point d'accès enregistré et si la connexion n'est pas possible, activate le mode point d'accès. Une fois qu'un nouvel identifiant/mot de passe est saisi la base essaie à nouveau de se connecter au réseau Wifi.</p>
<h3>Tâche Internet</h3>
<p>La tâche Internet gère la connexion entre la base et le serveur.
Lorsque l'Ethernet ou le Wifi est connecté, la tâche Internet communique avec le serveur web et vérifie que la base est bien activée et dans le cas contraire génère un code d'activation à utiliser sur le site web.
Lorsque des données sont prêtes à être transférer depuis la base, elle se connecte au serveur web et envoie les fichiers.
</p>
<h3>Tâche Bluetooth</h3>
<p>La tâche Bluetooth gère la communication en Bluetooth, elle est en attente de détection du capteur. Lorsque le capteur est posé sur la base, la tâche se réveille et active le Bluetooth pour tenter de se connecter au capteur.
Lorsque la connexion a été établi, elle envoie l'heure puis attend de recevoir tous les fichiers depuis le capteur. Les fichiers récupérés sont enregistrés dans le stockage local de la base, en attente d'être transféré sur internet par la tâche Internet</p>
<h3>Tâche Affichage</h3>
<p>La tâche affichage gère l'écran de la base. Elle le met à jour en fonction de l'état des différents composants de la base (Ethernet et Wifi) et en fonction de l'état (transfert Bluetooth, transfert Wifi, authentification web)</p>
<h2>Configuration du programme</h2>
<p>Certains fichiers .h sont modifiables afin de changer la configuration du programme :
<ul>
  <li><b>BaseBluetooth.h</b> : 
    <ul>
      <li><b>PIN_SENSOR_DETECT</b> : Lorsque ce pin est mis à HIGH, lance le transfert des données en Bluetooth (par défaut 17, le pin sur lequel est branché le reed switch)</li>
      <li><b>DEBUG_BLUETOOTH_SYNC</b> : Affichage des informations de débogage dans la console série (commenter pour désactiver)</li>
    </ul>
  </li>
  <li><b>BaseDisplay.h</b> :
    <ul><b>FLIP_SCREEN_VERTICALLY</b> : Retourner l'écran verticalement (par défaut False)</ul>
    <ul><b>PIN_SDA</b> : Pin SDA de l'écran (par défaut SDA)</ul>
    <ul><b>PIN_SCL</b> : Pin SCL de l'écran (par défaut SCL)</ul>
  </li>
  <li><b>BaseEthernet.h :</b>
    <ul>
      <li><b>ETHERNET_MAC</b> : Adresse MAC de l'Ethernet</li>
      <li><b>ETHERNET_CS_PIN</b> : Pin CS (Chip Select) de l'Ethernet (par défaut 25)</li>
      <li><b>ETHERNET_RESET_PIN</b> : Pin reset de l'Ethernet (par défaut 26)</li>
      <li><b>SYNC_SERVER_URL</b> : Adresse du serveur web (par défaut caspa.icare.univ-lille.fr)</li>
      <li><b>SYNC_SERVER_PORT</b> : Port de connexion au serveur web (par défaut 443)</li>
      <li><b>SYNC_SERVER_USE_HTTPS</b> : Le serveur web utilise l'HTTPS (commenter pour désactiver)</li>
      <li><b>DEBUG_ETHERNET</b> : Affichage des informations de débogage dans la console série (commenter pour désactiver)</li>
    </ul>
  </li>
  <li><b>BaseWifi.h :</b>
    <ul>
      <li><b>PORT_DNS</b> : Port du DNS du microcontrôleur (par défaut 53)</li>
      <li><b>PORT_SERVER</b> : Port du serveur du portail (par défaut 80)</li>
      <li><b>WIFI_SSID</b> : Nom du point d'accès (par défaut Caspa-PICO)</li>
      <li><b>WIFI_PASSWORD</b> : Mot de passe du point d'accès (par défaut Caspa123)</li>
      <li><b>SYNC_SERVER_URL</b> : Adresse du serveur web (par défaut caspa.icare.univ-lille.fr)</li>
      <li><b>SYNC_SERVER_PORT</b> : Port de connexion au serveur web (par défaut 443)</li>
      <li><b>SYNC_SERVER_USE_HTTPS</b> : Le serveur web utilise l'HTTPS (commenter pour désactiver)</li>
      <li><b>DEBUG_WIFI</b> : Affichage des informations de débogage dans la console série (commenter pour désactiver)</li>
    </ul>
  </li>
  <li><b>CustomHTTPClient.cpp</b> :
    <ul>
      <li><b>root_ca</b> : Contient le certificat racine du serveur web (pour le SSL)</li>
    </ul>
  <li><b>TA.H</b> : Contient les Trusted Anchors pour le SSL<br/>Pour plus d'information, voir <a href="https://github.com/OPEnSLab-OSU/SSLClient">ici</a> et <a href="https://github.com/OPEnSLab-OSU/SSLClient/blob/master/TrustAnchors.md">ici</a>
  </li>
</ul>
</p>
<h2>Montage de la base</h2>
<img src="https://i.ibb.co/0M8Bkfx/sch-ma-interconnexion.jpg" alt="sch-ma-interconnexion" border="0" />
