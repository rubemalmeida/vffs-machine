﻿# vffs-machine
Este projeto consiste em um sistema embarcado para controlar um processo automatizado de envase de líquidos. O sistema é baseado em um microcontrolador ESP32, que utiliza sensores e atuadores para realizar o controle do processo. O objetivo é automatizar e monitorar o envase de líquidos de diferentes tipos, como água, sucos de frutas etc.

## Funcionalidades
O sistema possui as seguintes funcionalidades:

1. **Botões de Controle:**
    - Botão de Parada Emergencial: Utilizado para interromper imediatamente o processo em caso de emergência.
    - Botão de Início de Calibração: Inicia o processo de calibração do sistema.
    - Botão de Início de Envase: Inicia o processo de envase do líquido.

2. **Indicadores LED:**
    - LED Branco: Indica o estado da conexão Wi-Fi. Pode estar aceso, piscando (em processo de conexão) ou apagado (desconectado).
    - LED Verde: Indica o estado do processo de envase. Fica aceso durante o envase e apaga quando o processo é concluído.

3. **Comunicação Wi-Fi:**
    - O sistema pode se conectar a uma rede Wi-Fi para permitir o monitoramento e controle remoto.

4. **Web Server:**
    - Um servidor web embutido permite configurar parâmetros do sistema, como tempo de selagem, volume a ser envasado e tipo de líquido.

5. **Medidor de Fluxo:**
    - Utiliza um medidor de fluxo para medir a quantidade de líquido sendo envasado.

6. **Processo de Calibração:**
    - Permite calibrar o sistema para diferentes tipos de líquido e garantir uma medição precisa.

7. **Processo de Envase:**
    - Controla a abertura das válvulas e o acionamento do motor da bomba para realizar o envase do líquido.

8. **Processo de Selagem:**
    - Após o envase, controla a selagem do recipiente.

## Descrição dos Processos

### 1. Processo de Calibração
- Inicia quando o botão de calibração é pressionado.
- Durante o processo, o sistema registra a quantidade de pulsos do medidor de fluxo para calibração.
- Ao final, o sistema calcula o fator de calibração com base na quantidade de pulsos registrados.

### 2. Processo de Envase
- Inicia quando o botão de envase é pressionado.
- O sistema abre a válvula de envase e aciona o motor da bomba para iniciar o fluxo do líquido.
- Monitora a quantidade de líquido envasado com base nos pulsos do medidor de fluxo.
- Encerra o processo quando o volume desejado é alcançado ou quando o número de pulsos é atingido.

### 3. Processo de Selagem
- Após o envase, o sistema pode controlar a selagem do recipiente, se aplicável.
- A selagem é realizada por um tempo pré-definido.

### 4. Parada Emergencial
- Um botão de parada emergencial permite interromper imediatamente qualquer processo em caso de emergência.

## Configuração do Projeto
Para configurar o projeto, basta acessar a interface web do sistema e inserir os parâmetros desejados, como tempo de selagem, volume a ser envasado e tipo de líquido.

## Observações
- Todo o circuito é alimentado por 3 fontes CC, sendo uma chaveada 12V 5A, outra 12V 1A e uma 5v 1A.
- A fonte chaveada é responsável pela alimentação da bomba de transmissão bem como sensor e valvulas solenoides eletropneumática.
- A fonte 12V 1A é reponsável pelos leds indicativos.
- E a fonte 5A 1A é específica para o microcontrolador.
- O sistema é alimentado por um microcontrolador ESP32.
- Os sensores e atuadores são conectados aos pinos do ESP32 conforme definido no código.
- A comunicação com o servidor web é realizada através do protocolo HTTP.

Este projeto fornece uma solução flexível e automatizada para o processo de envase de líquidos, garantindo precisão e eficiência.
