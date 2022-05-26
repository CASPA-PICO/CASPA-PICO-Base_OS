<h1>CASPA-PICO Capteur OS</h1>
<p>Le programme est un projet <a href="https://platformio.org/">Platform IO</a> testé sur une ESP32 modèle : <a href="https://www.dfrobot.com/product-2231.html">DFR0654-F</a><br/>
Il nécessite <b>obligatoirement</b> la bibliothèque <a href="https://github.com/CASPA-PICO/PLTP">PLTP</a> pour fonctionner.</p>
<h2>Bibliothèques utilisées</h2>
<ul>
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
<h2>Montage de la base</h2>
<img src="https://i.ibb.co/0M8Bkfx/sch-ma-interconnexion.jpg" alt="sch-ma-interconnexion" border="0" />
