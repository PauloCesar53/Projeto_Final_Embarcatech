# Projeto_Final_Embarcatech
Repositório criado para versionamento do projeto final da residência em software embarcado 


## Descrição do Funcionamento do programa 
O programa simula sensores ADC, por meio do joystick, com eixo X representando um sensor de umidade
e o eixo Y representando sensor de temperatura. Após iniciar a execução do programa será mostrado a 
leitura em tempo real dos sensores no Display SSD1306, e na matriz de LEDs a representação percentual
do nível de umidade na cor azul (pode ser alterado para nível de temperatura com botão B representado
pela cor vermelha). 
Ao atingir o percentual de umidade de 20% ocorre o acionamento do LED comum azul, que simula uma bomba 
d'água, ficando acesso até que a umidade chegue a 60%. Caso a umidade chegue a 15% um alerta sonoro é emitido 
(função que pode ser desativada com botão do Joy Stick, e verificada no display na tela inicial). Ao 
acionar o botão A é mostrado o histórico da máxima temperatura observada no Display em 30 intervalos distintos 
(que para efeito de simulação foi considerado intervalos de 5 segundos). 

## Compilação e Execução

1. Certifique-se de que o SDK do Raspberry Pi Pico está configurado no seu ambiente.
2. Compile o programa utilizando a extensão **Raspberry Pi Pico Project** no VS Code:
   - Abra o projeto no VS Code, na pasta PROJETO_FINAL_EMBARCATECH tem os arquivos necessários para importar 
   o projeto com a extensão **Raspberry Pi Pico Project**.
   - Vá até a extensão do **Raspberry pi pico project** e após importar (escolher sdk 2.1.0) os projetos  clique em **Compile Project**.
3. Coloque a placa em modo BOOTSEL e copie o arquivo `Projeto.uf2`  que está na pasta build, para a BitDogLab conectado via USB.


**OBS1: Devem importar os projetos para gerar a pasta build, pois a mesma não foi inserida no repositório**
**OBS2: Em algumas versões da BitDogLab o posicionamento dos eixos X e Y podem estar invertidos**
**OBS3: O projeto foi produzido com base na versão do sdk 2.1.0, considerar esse fato quando for importar o projeto**



## Emulação com Wokwi

Para testar os programas sem hardware físico, você pode utilizar o **Wokwi** para emulação no VS Code:

1. Instale a extensão **Wokwi for VS Code**
3. Inicie a emulação:
   - Clique no arquivo `diagram.json` e inicie a emulação.
4. Teste o funcionamento do programa diretamente no ambiente emulado.
   
**OBS: Os arquivos diagram.json e wokwi.toml foram inseridos para o projeto.**

## Link com demonstração no youtube

Demonstração do funcionamento do projeto na BitDogLab: (https://youtu.be/s7z3Uz5vrYA?si=6Q_leHPckCi1QKfC)


## Colaboradores
- [PauloCesar53 - Paulo César de Jesus Di Lauro ] (https://github.com/PauloCesar53)
