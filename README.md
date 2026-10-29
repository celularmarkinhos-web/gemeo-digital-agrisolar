# Gêmeo Digital AgriSolar com Edge AI (ESP32-S3)

Repositório oficial de código-fonte e materiais suplementares vinculados à pesquisa voltada ao desenvolvimento de sistemas de monitoramento e proteção para microredes isoladas na agricultura familiar.

## 📌 Sobre o Projeto
Este projeto implementa uma estação de energia solar inteligente baseada no microcontrolador **ESP32-S3**, integrando sensores de corrente/tensão (INA219), sensores ambientais (DHT22), processamento local por **Edge AI (Heurísticas embarcadas)** e visualização via **Gêmeo Digital (Digital Twin)** utilizando o protocolo MQTT e um painel web interativo.

O sistema foi desenhado sob a premissa da **Inovação Frugal**, priorizando baixo custo de implantação e alta resiliência operacional (capaz de operar de forma autônoma mesmo na ausência de conexão com a internet).

## 📂 Estrutura do Repositório
* **`/firmware`**: Contém o código-fonte em C++ (`.ino`) desenvolvido para o ESP32-S3 na IDE Arduino, estruturado com rotinas não-bloqueantes e acionamento hierárquico de cargas.
* **`/web-dashboard`**: Contém a interface web interativa do gêmeo digital, responsável por exibir telemetria em tempo real, gráficos dinâmicos e registro de eventos/anomalias da IA.

## 🛠️ Tecnologias Utilizadas
* **Hardware:** ESP32-S3 N16R8, Sensores INA219 (I²C), Sensores DHT22.
* **Linguagem:** C++ (Arduino IDE).
* **Protocolos e Conectividade:** Wi-Fi, MQTT (Broker HiveMQ).
* **Interface:** HTML5, JavaScript, CSS (Painel Web responsivo).

## 📄 Licença
Este projeto é disponibilizado para fins acadêmicos e de reprodução científica sob a licença MIT.
