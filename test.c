#include <stdio.h>
#include <unistd.h>

int main(){
while(1){
int c = getch();
printf("%d", c);
}
return 0;
}
