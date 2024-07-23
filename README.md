# kakashi
Poorly made fork of the C++ server for Attorney Online 2 with additional functional. Used on AO Shapeshifter (RU).
For a better understanding the work of this program it is recommended to read the wiki, located in the same repo.

Плохо сделанный форк сервера для Attorney Online 2, написанного на C++, с дополнительным функционалом. Используется на AO Shapeshifter (RU).
Для лучшего понимания работы данной программы рекомендуется прочитать вики, расположенное в этом же репозитории.

# Build instructions

Required Qt => 5.10 and Qt Websockets

```
   #Installing dependencies (Qt5)
   #Установка зависимостей (Qt5)
   #Ubuntu 20.04:
   sudo apt-get install qt5-default gcc g++ libqt5websockets5-dev

   #Ubuntu 22.04/Debian 11:
   sudo apt-get install qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools gcc g++ libqt5websockets5-dev

   #Installing dependencies (Qt6)
   #Установка зависимостей (Qt6)

   #Ubuntu 22.04/Debian 11:
   #If you are using Debian to build, you will need Debian Backports
   #Если для компиляции используется Debian, то вам потребуется Debian Backports
   sudo apt-get install qt6-base-dev gcc g++ libqt6websockets6-dev

   #Building
   #Компиляция
   git clone https://github.com/Ddedinya/kakashi
   cd kakashi
   qmake (qt5)
   qmake6 (qt6)
   make
```

akashi is created by scatterflower, Sananto, in1tiate, MangosArentLiterature and other cool people.
This fork is supported by Ddedinya.

akashi создан scatterflower, Sananto, in1tiate, MangosArentLiterature и другими крутыми людьми.
Данный форк поддерживается Ddedinya, т.е. мной, агада.
