#include "person.h"

int num_of_datas()
{
    QFile file("C:/Users/ilker/Desktop/database.txt");
   if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
       return 0;
   int count = 0;
   QTextStream in(&file);
   while (!in.atEnd())
   {
       ++count;
   }
   return count;
}

