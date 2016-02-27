# InvShow
InvShow jest wtyczką do komunikatora [AQQ](http://www.aqq.eu/pl.php). Zmienia stan kontaktu z "rozłączony" na "niewidoczny" podczas rozmowy. Stan kontaktu jest przywracany, gdy rozłączy się on z siecią, zmieni swój stan lub po upływie 5 minut od zamknięcia okna rozmowy.

### Wymagania
Do skompilowania wtyczki AQQ Restarter potrzebne jest:

* Embarcadero RAD Studio XE7 (tylko C++Builder)
* [Plik nagłówkowy SDK komunikatora AQQ](https://bitbucket.org/beherit/pluginapi-for-aqq-im)
* Opcjonalnie [UPX](http://upx.sourceforge.net/) dla zmniejszenia rozmiaru pliku wynikowego (w szczególności wersji x64)

### Błędy
Znalezione błędy wtyczki należy zgłaszać na [tracekerze](http://forum.aqq.eu/tracker/project-101-invshow/) znajdującym na oficjalnym forum komunikatora AQQ lub pisząc bezpośrednio do autora wtyczki (preferowany kontakt poprzez Jabber).

### Kontakt z autorem
Autorem wtyczki InvShow jest Krzysztof Grochocki. Możesz skontaktować się z nim poprzez XMPP pisząc na im@beherit.pl.

### Licencja
Wtyczka InvShow objęta jest licencją [GNU General Public License 3](http://www.gnu.org/copyleft/gpl.html).

~~~~
InvShow
Copyright (C) 2010-2016  Krzysztof Grochocki

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
~~~~