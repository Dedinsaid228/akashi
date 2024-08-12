# kakashi
Poorly made fork of the C++ server for Attorney Online 2 with additional functional. Used on the AO Shapeshifter (RU) server.
For a better understanding the work of this program it is recommended to read the wiki, located in the same repo.

Плохо сделанный форк сервера для Attorney Online 2, написанного на C++, с дополнительным функционалом. Используется на сервере AO Shapeshifter (RU).
Для лучшего понимания работы данной программы рекомендуется прочитать вики, расположенное в этом же репозитории.

# Build instructions

Required Qt6 and Qt Websockets

```
   #Installing dependencies
   #Установка зависимостей

   #Ubuntu 22.04/Debian 11:
   #If you are using Debian to build, you probaly will need Debian Backports
   #Если для компиляции используется Debian, то вам может потребоваться Debian Backports
   sudo apt-get install qt6-base-dev gcc g++ libqt6websockets6-dev

   #Building
   #Компиляция
   git clone https://github.com/Ddedinya/kakashi
   cd kakashi
   qmake6
   make
```

akashi is created by scatterflower, Sananto, in1tiate, MangosArentLiterature and other cool people.
This fork is supported by Ddedinya.

akashi создан scatterflower, Sananto, in1tiate, MangosArentLiterature и другими крутыми людьми.
Данный форк поддерживается Ddedinya, т.е. мной, агада.
