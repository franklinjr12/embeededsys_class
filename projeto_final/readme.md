# Descrição

A pasta pc_version tem o código para controlar o carro do simulador em um código para Windows com porta serial simulada;

A pasta board_version tem o código para controlar o carro do simulador em um código para Tiva com porta serial usando UART;

O sistema assume que o simulador está iniciado e parado para começar a operação. É dada uma valocidade inicial para o carro definida no código e uma angulação menor que 10 unidades. A partir dai o controlador PID entra em ação e tenta manter o carro no centro da pista. O sistema funciona com velocidades até 6 unidades, para mais do que isso não é garantido o funcionamento.

Devido a limitações do simulador não foi possível tornar o sistema rápido o suficiente, foi necessário um atraso de 300ms entre cada mensagem, prejudicando o funcionamento do controlador.

Existem ainda 2 problemas com o simulador que são:
- Após algumas dezenas de mensagens o simulador fica extremamente lento e inutilizável;
- Após completar a primeira curva existe algum problema na leitura do sensor de RF que causa o controlador a ficar instável;
