# Hub météo

Horloge et météo de Paris sur une Freenove ESP32-S3. L'heure vient d'Internet, la météo d'Open-Meteo. La position est fixe : 48,86 °N, 2,35 °E, fuseau de Paris.

Le même programme a deux rendus. En développement, les trois lignes sortent dans le moniteur série du PC. En production, le même texte est dessiné sur l'écran.

```
13°C - Couvert
18:11:12
Mar 27 oct 2026
```

L'engrenage en bas à droite ouvre les réglages : accueil, ou luminosité. La caméra n'est pas utilisée.

## Technique

| Rôle | Choix |
|---|---|
| Carte | Freenove ESP32-S3-WROOM (FNK0085), programmée comme ESP32S3 Dev Module |
| Langage | Arduino (C++) |
| IDE | Arduino IDE 2, core esp32 d'Espressif |
| Heure | NTP (`pool.ntp.org`, `time.google.com`) |
| Météo | [Open-Meteo](https://api.open-meteo.com/), sans clé |
| Écran | Adafruit GFX et Adafruit ILI9341, bus SPI |
| Tactile | XPT2046, bus SPI à part |
| Console | moniteur série à 115200 bauds |

Le Wi-Fi sert à l'heure et à la météo. Le câble USB alimente la carte et affiche la console. `config.h` est ignoré par git : copier `config.h.exemple` vers `config.h`, puis y mettre le nom et le mot de passe du réseau.

## Matériel

- Freenove ESP32-S3-WROOM, avec ses deux prises USB
- écran IPS 3,2 pouces, 320×240, contrôleur ILI9341, avec tactile
- fils Dupont femelle-femelle

La couleur des fils ne compte pas. Elle sert seulement à les distinguer. La nappe caméra reste débranchée : ses broches servent au tactile.

Avant de brancher `VCC`, lire le marquage de l'écran. S'il indique 3,3 V à 5 V, le fil va sur `5V`. S'il indique seulement `3V3`, ce fil va sur `3V3`. Le fil `LED` va sur `16` : c'est lui qui règle la luminosité.

## Branchement

Carte débranchée de l'USB. Quatorze fils, d'après les noms imprimés sur l'écran et sur la Freenove. Les neuf premiers dessinent l'image. Les cinq suivants lisent le doigt.

| Écran | Freenove | Rôle |
|---|---|---|
| VCC | 5V | alimentation |
| GND | GND | masse |
| LED | 16 | rétroéclairage, de 5 à 100 % |
| CS | 14 | sélection de l'écran |
| RESET | 21 | remise à zéro |
| DC (ou RS) | 47 | commande ou pixel |
| SDI (ou MOSI) | 1 | données vers l'écran |
| SCK | 2 | horloge |
| SDO (ou MISO) | 41 | données vers l'ESP32 |
| T_CLK | 4 | horloge du tactile |
| T_CS | 5 | sélection du tactile |
| T_DIN | 6 | données vers le tactile |
| T_DO | 7 | données vers l'ESP32 |
| T_IRQ | 15 | le doigt est posé |

La broche `2` commande aussi la LED de la carte. Elle peut luire pendant le rafraîchissement.

Antenne de la Freenove en haut : `3V3` est en haut à gauche, `5V` en bas à gauche, `GND` en bas à droite. `1`, `2` et `41` sont en haut à droite, `47` et `21` plus bas du même côté. `4`, `5`, `6`, `7` et `15` sont à gauche, juste sous `RST`, dans cet ordre. `16` est la broche juste sous `15`.

Le tactile a son propre bus. Il ne reprend pas `SCK`, `SDI` et `SDO` : chaque broche de la Freenove ne reçoit qu'un fil.

Après le branchement, passer en mode écran et retéléverser. Le rétroéclairage allumé avec l'heure lisible confirme les fils de l'image. Un écran allumé mais vide ou brouillé indique un fil parmi `CS`, `RESET`, `DC`, `SDI` et `SCK` inversé. Un écran noir indique `VCC`, `GND` ou `LED` au mauvais endroit.

L'engrenage en bas à droite ouvre les réglages. Deux boutons : Accueil, et Luminosité. La page luminosité a les boutons - et +, par pas de 5 %, de 5 % à 100 %, et un bouton Auto avec un soleil. Auto suit le jour et la nuit : 75 % le jour, 25 % la nuit. Un appui change de 5 %. Le garder enfoncé continue. + ou - passe en manuel. Le choix est gardé au relâchement.

## Développement et production

Le choix est la ligne `APP_MODE` de `config.h`.

| | Développement | Production |
|---|---|---|
| Valeur | `MODE_CONSOLE` | `MODE_SCREEN` |
| Où ça s'affiche | moniteur série du PC | écran ILI9341 |
| Écran requis | non | oui, branché comme ci-dessus |
| Code écran | non compilé | compilé |

Les deux modes envoient la même chose : température, heure `HH:MM:SS`, date en français. La météo est relue toutes les 10 minutes. L'heure avance chaque seconde.

Pour passer de l'un à l'autre, modifier `APP_MODE`, puis téléverser à nouveau sur la carte.
