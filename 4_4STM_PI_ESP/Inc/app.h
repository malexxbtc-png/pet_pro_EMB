#ifndef APP_H
#define APP_H

void App_Init(void);   /* викликати в main() після ініціалізації периферії */
void App_Loop(void);   /* викликати в while(1) без жодних затримок */

#endif /* APP_H */
