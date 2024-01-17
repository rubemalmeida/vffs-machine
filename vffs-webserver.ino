#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <TaskScheduler.h>

// Variáveis dos pinos de entrada e saída e controle de tempo
#define PIN_BOTAO_EMERGENCIAL 26 // botão para parada emergencial
#define PIN_BOTAO_CALIBRACAO 25  // botão para calibração da máquina
#define PIN_BOTAO_PROCESSO 27    // botão para iniciar o processo de envase
#define PIN_LED_VERDE 4          // led indicativo para o processo em andamento
#define PIN_LED_BRANCO 5         // caso o led esteja aceso, indica que a máquina está ligada e conectada ao Wi-Fim, \
                                  // e caso esteja piscando, indica que a máquina está ligada, porém, não está conectada ao Wi-Fi
#define PIN_VALVULA_LIBERACAO 18 // RL4IN1 - LIBERACAO - válvula pneumatica para controle do fluxo de ar
#define PIN_VALVULA_ENVASE 19    // RL4IN2 - ENVASE - válvula pneumatica para controle do fluxo do fluido a ser envasado
#define PIN_VALVULA_SELAGEM 21   // RL4IN3 - SELAGEM - válvula pneumatica para controle do pistão de selagem
#define PIN_MOTOR_BOMBA 22       // RL4IN4 - BOMBA - motor para controle transporte do fluido a ser envasado
#define PIN_MEDIDOR_FLUXO 23     // medidor de fluxo para controle do volume de fluido a ser envasado

#define TURN_ON 0
#define TURN_OFF 1

#define PIN_CONTROL(pinValue, state) pinControl(pinValue, state, __func__)

const char *pin_names[] = {
    "",                      // 0
    "",                      // 1
    "",                      // 2
    "",                      // 3
    "PIN_LED_VERDE",         // 4
    "PIN_LED_BRANCO",        // 5
    "",                      // 6
    "",                      // 7
    "",                      // 8
    "",                      // 9
    "",                      // 10
    "",                      // 11
    "",                      // 12
    "",                      // 13
    "",                      // 14
    "",                      // 15
    "",                      // 16
    "",                      // 17
    "PIN_VALVULA_LIBERACAO", // 18
    "PIN_VALVULA_ENVASE",    // 19
    "",                      // 20
    "PIN_VALVULA_SELAGEM",   // 21
    "PIN_MOTOR_BOMBA",       // 22
    "PIN_MEDIDOR_FLUXO",     // 23
    "",                      // 24
    "PIN_BOTAO_CALIBRACAO",  // 25
    "PIN_BOTAO_EMERGENCIAL", // 26
    "PIN_BOTAO_PROCESSO"     // 27
};

const int pinSize = sizeof(pin_names) / sizeof(pin_names[0]);

// Configuração do web server
const String hostname = SECRET_HOSTNAME;
const char *ssid = SECRET_SSID;
const char *password = SECRET_WIFI_PASSWORD;

AsyncWebServer server(80);

IPAddress local_IP(192, 168, 1, 70);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 1, 1);
IPAddress secondaryDNS(8, 8, 8, 8);

const int INTERVALO_MINIMO = 3000; // intervalo mínimo entre acionamentos do botão (em milisegundos)
const String SUCESSO = "SUCESSO";
const String ERROR = "ERROR";

// Variáveis globais a serem alteradas durante o processo
int btnProcEstadoCorrente = HIGH; // the current reading from the input pin
int btnProcUltimoEstado = LOW;    // o estado anterior do botão de processo
int btnProcUltimoEstadoEfetivo = LOW;

unsigned long btnProcStartAcionamento = 0;       // Adicionado para rastrear o tempo de pressionamento do botão
unsigned long btnProcTempoUltimoAcionamento = 0; // tempo em milisegundos do último acionamento do botão

int btnCalibUltimoEstadoEfetivo = LOW;
int btnCalibUltimoEstado = LOW;                   // o estado anterior do botão de processo
int btnCalibEstadoCorrente = HIGH;                // the current reading from the input pin
unsigned long btnCalibTempoUltimoAcionamento = 0; // tempo em milisegundos do último acionamento do botão

unsigned long inicioSelagem = 0; // tempo em milisegundos do início da selagem

unsigned long btnParadaTempoUltimoAcionamento = 0; // tempo em milisegundos do último acionamento do botão
unsigned long btnParadaStartAcionamento = 0;       // Adicionado para rastrear o tempo de pressionamento do botão
bool paradaEmergencial = false;                    // Flag para indicar se a parada emergencial foi acionada

unsigned long ultimaNotificaoConexao = 0; // tempo em milisegundos da última notificação de conexão
int tempoNotificacaoConexao = 1;          // intervalo de tempo em milisegundos para notificação de conexão
int statusWifi = WL_DISCONNECTED;         // status da conexão Wi-Fi
bool primeiraConexao = true;              // Flag para indicar se é a primeira conexão
bool statusLedBranco = false;             // Flag para indicar se o led branco está aceso ou apagado
bool statusLedVerde = false;              // Flag para indicar se o led verde está aceso ou apagado

bool modoCalibracao = false; // Flag para indicar se está em modo de calibração
bool modoEnvase = false;     // Flag para indicar se está em modo de envase
bool modoSelagem = false;    // Flag para indicar se está em modo de selagem

int qtdEnvaseRealizado = 0; // Contador de vezes que o processo foi executado
// int vazao = 0;              // Vazão do fluido em processo de envase
// int volume = 0;             // Volume do fluido em processo de envase

volatile byte qtdPulsoCalibracao = 0; // Contador de pulsos do medidor de fluxo usado na calibração
volatile byte qtdPulso = 0;           // Contador de pulsos do medidor de fluxo
int qtdPulsoAMonitorar = 0;           // Quantidade de pulsos a monitorar
byte pulse1Sec = 0;
float fatorCalibracao = 0.0; // Fator de calibracao do medidor de fluxo
float tempoSelagem = 0.0;    // Tempo de selagem do pistão em segundos
int volumeAserEnvasado = 0;  // Volume a ser envasado
long medicaoPreviousMillis = 0;
int intervalMedicao = 1000;
bool medicaoHabilitada = false;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;

int repeticoesPiscaBranco = 0;
int repeticoesPiscaVerde = 0;
int intervalo500 = 500;
int intervalo1000 = 1000;
int intervaloPisca = 0;

String eventos = ""; // String para armazenar os eventos ocorridos

Scheduler runner; // Scheduler para gerenciar as tarefas

// Código html do web server
const char index_html[] PROGMEM = R"rawliteral(%HTML%)rawliteral";

void checkPiscaVerde();
void checkWifi();
void checkCalibracao();
void checkEnvase();
void checkParadaEmergencial();
void checkMedicao();
void calibracao(bool btnPressionado);
void envase();

Task TarefaWifi(500, TASK_FOREVER, &checkWifi);
Task TarefaCalibracao(100, TASK_FOREVER, &checkCalibracao);
Task TarefaEnvase(100, TASK_FOREVER, &checkEnvase);
Task TarefaParadaEmergencial(100, TASK_FOREVER, &checkParadaEmergencial);
Task TarefaMedicao(100, TASK_FOREVER, &checkMedicao);
Task TarefaPiscaVerde(500, TASK_FOREVER, &checkPiscaVerde);

void log(String evento)
{
    eventos += evento + ";";
    Serial.println(evento);
}

// void pinControl(int pinValue, int state)
void pinControl(int pinValue, int state, const char *callingFunction)
{
    String stateName = state == TURN_ON ? "ON" : "OFF";
    String pinName = "PIN" + String(pinValue);
    if (pinValue >= 0 && pinValue < pinSize)
    {
        if (pin_names[pinValue] == "")
            pinName += "-NÃO DEFINIDO";
        else if (state == TURN_ON || state == TURN_OFF)
        {
            pinName = pin_names[pinValue];
            digitalWrite(pinValue, state);
            if (pinValue == PIN_LED_BRANCO)
                statusLedBranco = state == TURN_ON ? true : false;
            else if (pinValue == PIN_LED_VERDE)
                statusLedVerde = state == TURN_ON ? true : false;
            // if (pinValue == PIN_LED_BRANCO || pinValue == PIN_LED_VERDE)
            //   return;
        }
        else
            stateName += "ESTADO INVALIDO";
    }
    else
        pinName += "-DESCONHECIDO";

    log((String)callingFunction + " > " + pinName + ": " + stateName);
}

void waitDelay(unsigned long milliseconds)
{
    unsigned long start = millis();
    while (millis() - start < milliseconds)
    {
        delay(1);
        yield();
    }
}

String processor(const String &var)
{
    if (var == "PULSOS-CALIB")
        return String(qtdPulso) + "/" + String(qtdPulsoAMonitorar);

    if (var == "FATOR-CALIBRACAO" && fatorCalibracao > 0)
        return String(fatorCalibracao);

    if (var == "TEMPO-SELAGEM")
        return String(tempoSelagem);

    if (var == "VOLUME-A-SER-ENVASADO")
        return String(volumeAserEnvasado);

    if (var == "HIDDEN-FORM-MEDICAO") // && qtdPulsoCalibracao > 0 && qtdPulsoAMonitorar > 0)
        return String("hidden");

    if (var == "HIDDEN-FORM-FATOR-CALIBRACAO") // && qtdPulsoCalibracao > 0)
        return String("hidden");

    if (var == "HIDDEN-FORM-PARAMETRO") //&& fatorCalibracao == 0)
        return String("hidden");

    return String();
}

void checkPiscaVerde()
{
    if (repeticoesPiscaVerde == 0)
    {
        TarefaPiscaVerde.disable();
        return;
    }

    if ((intervaloPisca == intervalo1000) && ((repeticoesPiscaVerde % 2) == 0))
        PIN_CONTROL(PIN_LED_VERDE, (statusLedVerde ? TURN_OFF : TURN_ON));

    repeticoesPiscaVerde--;
}

void piscarResposta(String action)
{
    if (action != "SUCESSO" && action != "ERROR")
        return;

    if (action == "SUCESSO")
        intervaloPisca == intervalo500;
    else // ERROR
        intervaloPisca = intervalo1000;

    repeticoesPiscaVerde = 6 * (intervaloPisca / 500);
    TarefaPiscaVerde.restart();
}

void checkWifi()
{
    int tempStatusWifi = WiFi.status();
    if (tempStatusWifi != WL_CONNECTED)
    {
        statusWifi = tempStatusWifi;
        if ((millis() - ultimaNotificaoConexao) > tempoNotificacaoConexao)
        {
            WiFi.begin(ssid, password);
            tempoNotificacaoConexao = tempoNotificacaoConexao == 1 ? 5000 : tempoNotificacaoConexao * 3;
            ultimaNotificaoConexao = millis();
            if (!primeiraConexao)
                log("WiFi desconectado, tentando estabelecer conexão... (" + String(tempoNotificacaoConexao / 1000) + "s)");
        }
        PIN_CONTROL(PIN_LED_BRANCO, (statusLedBranco ? TURN_OFF : TURN_ON));
    }
    else if (tempStatusWifi == WL_CONNECTED && statusWifi != WL_CONNECTED)
    {
        primeiraConexao = false;
        tempoNotificacaoConexao = 1;
        statusWifi = WL_CONNECTED;
        log("WiFi conectado, endereço IP: " + String(WiFi.localIP().toString().c_str()));
        if (statusLedBranco == false)
            PIN_CONTROL(PIN_LED_BRANCO, TURN_ON);
    }
}

void checkCalibracao()
{
    static unsigned long btnCalibTempoPrimeiroPressionado = 0;

    // Processo de calibração
    btnCalibEstadoCorrente = digitalRead(PIN_BOTAO_CALIBRACAO);
    if (
        (btnCalibUltimoEstado == HIGH) &&
        (btnCalibEstadoCorrente == LOW) &&
        paradaEmergencial == false)
    {
        if (btnCalibTempoPrimeiroPressionado == 0) // Primeira vez que o botão é pressionado
        {
            btnCalibTempoPrimeiroPressionado = millis();
        }
        else if ((millis() - btnCalibTempoPrimeiroPressionado) >= 500) // Botão mantido pressionado por 500ms
        {
            btnCalibTempoPrimeiroPressionado = 0; // Reset the first pressed time
            if ((millis() - btnCalibTempoUltimoAcionamento) >= INTERVALO_MINIMO)
            {
                log("Botão de calibração pressionado");
                btnCalibTempoUltimoAcionamento = millis();
                btnCalibUltimoEstadoEfetivo = HIGH;
                calibracao(true);
            }
        }
    }
    else if (
        ((btnCalibUltimoEstado == LOW) &&
         (btnCalibEstadoCorrente == HIGH) &&
         (btnCalibUltimoEstadoEfetivo == HIGH)) ||
        paradaEmergencial == true)
    {
        log("Botão de calibração liberado");
        btnCalibUltimoEstadoEfetivo = LOW;
        btnCalibTempoPrimeiroPressionado = 0; // Reset the first pressed time
        calibracao(false);
        if (paradaEmergencial == true)
            paradaEmergencial = false;
    }

    if (btnCalibUltimoEstado != btnCalibEstadoCorrente)
        btnCalibUltimoEstado = btnCalibEstadoCorrente;
}

void checkEnvase()
{
    static unsigned long btnProcStartAcionamento = 0;

    // Processo de envase
    btnProcEstadoCorrente = digitalRead(PIN_BOTAO_PROCESSO);
    if (
        (btnProcUltimoEstado == HIGH) &&
        (btnProcEstadoCorrente == LOW))
    {
        if (btnProcStartAcionamento == 0) // Primeira vez que o botão é pressionado
            btnProcStartAcionamento = millis();
        else if ((millis() - btnProcStartAcionamento) >= 500) // Botão mantido pressionado por 500ms
        {
            btnProcStartAcionamento = 0; // Reset the first pressed time
            if ((millis() - btnProcTempoUltimoAcionamento) >= INTERVALO_MINIMO)
            {
                log("Botão de envase pressionado");
                btnProcTempoUltimoAcionamento = millis();
                btnProcUltimoEstadoEfetivo = HIGH;
                envase();
            }
        }
    }
    else if (
        (btnProcUltimoEstado == LOW) &&
        (btnProcEstadoCorrente == HIGH) &&
        (btnProcUltimoEstadoEfetivo == HIGH))
    {
        btnProcUltimoEstadoEfetivo = LOW;
        btnProcStartAcionamento = 0; // Reset the first pressed time
    }

    if (btnProcUltimoEstado != btnProcEstadoCorrente)
        btnProcUltimoEstado = btnProcEstadoCorrente;
}

void checkSelagem()
{
    if (modoSelagem == true && (millis() - inicioSelagem) >= (tempoSelagem * 1000)
    {
        PIN_CONTROL(PIN_VALVULA_SELAGEM, TURN_OFF);
        waitDelay(1000);
        PIN_CONTROL(PIN_VALVULA_LIBERACAO, TURN_OFF);

        qtdEnvaseRealizado++;
    }

    if (modoSelagem == true)
        return;

    log("Selando... (" + String(tempoSelagem) + "s)");
    modoSelagem = true;
    PIN_CONTORL(PIN_VALVULA_SELAGEM, TURN_ON);
    inicioSelagem = millis();
    while ((millis() - inicioSelagem) < (tempoSelagem * 1000))
    {
        if (paradaEmergencial)
        {
            paradaEmergencial = false;
            break;
        }
        waitDelay(100);
    }
}

void checkParadaEmergencial()
{
    if (!modoCalibracao && !modoEnvase && !modoSelagem)
        return;

    bool buttonState = digitalRead(PIN_BOTAO_EMERGENCIAL);
    if (buttonState == LOW)
    {
        if (btnParadaStartAcionamento == 0) // Se o botão acabou de ser pressionado
        {
            btnParadaStartAcionamento = millis(); // Registre o tempo de pressionamento do botão
        }
        else if ((millis() - btnParadaStartAcionamento) > 100) // Se o botão foi mantido pressionado por 500ms
        {
            btnParadaStartAcionamento = 0; // Reset the button press time

            if ((millis() - btnParadaTempoUltimoAcionamento) >= INTERVALO_MINIMO)
            {
                paradaEmergencial = true;
                btnParadaTempoUltimoAcionamento = millis();
                log("Parada emergencial acionada");
                // for (int i = 0; i < pinSize; i++)
                // {
                //     if (i == PIN_BOTAO_PROCESSO || i == PIN_BOTAO_CALIBRACAO || i == PIN_BOTAO_EMERGENCIAL || i == PIN_MEDIDOR_FLUXO)
                //         continue;

                //     if (pin_names[i] == "")
                //         continue;

                //     PIN_CONTROL(i, TURN_OFF);
                // }
                // piscarResposta(SUCESSO);
                // PIN_CONTROL(PIN_LED_BRANCO, TURN_ON);
            }
        }
    }
    else
    {
        btnParadaStartAcionamento = 0; // Reset the button press time if the button is not being pressed
    }
}

void checkMedicao()
{
    if (fatorCalibracao == 0)
        return;

    if (medicaoHabilitada && (millis() - medicaoPreviousMillis >= intervalMedicao))
    {
        pulse1Sec = qtdPulso;
        qtdPulso = 0;
        flowRate = ((1000.0 / (millis() - medicaoPreviousMillis)) * pulse1Sec) / fatorCalibracao; // in L/min
        medicaoPreviousMillis = millis();

        flowMilliLitres = (flowRate / 60) * 1000; // mL/s
        totalMilliLitres += flowMilliLitres;      // mL
    }

    if (modoEnvase)
        medicaoHabilitada = true;
    else
        medicaoHabilitada = false;
}

void salvar(AsyncWebServerRequest *request)
{
    int params = request->params();
    log("params: " + String(params));
    bool erro = false;
    for (int i = 0; i < params; i++)
    {
        AsyncWebParameter *p = request->getParam(i);
        String parametro = p->name().c_str();
        String valor = p->value().c_str();
        if (valor == "" || valor == "undefined" || valor == "null" || valor == "0" || valor == "0.0")
        {
            log("Valor não definido. POST[" + parametro + "]: " + valor);
            erro = true;
        }
        else if (parametro == "input-medicao")
        {
            if (qtdPulsoCalibracao > 0)
            {
                int volumeDispensado = valor.toInt();
                fatorCalibracao = volumeDispensado / qtdPulsoCalibracao;
            }
            else
            {
                log("Calibração não identificada. Qtd de pulsos: " + String(qtdPulsoCalibracao));
                erro = true;
            }
        }
        else if (parametro == "input-fator-calibracao")
            fatorCalibracao = valor.toFloat();
        else if (parametro == "input-tempo-selagem")
            tempoSelagem = valor.toFloat();
        else if (parametro == "input-parametro")
        {
            if (fatorCalibracao > 0)
            {
                volumeAserEnvasado = valor.toInt();
                qtdPulsoAMonitorar = volumeAserEnvasado / fatorCalibracao;
            }
            else
            {
                log("Fator de calibracao pendente");
                erro = true;
            }
        }
        else
        {
            log("Parametro não reconhecido: " + parametro);
            erro = true;
        }
    }
    if (erro)
    {
        piscarResposta(ERROR);
        request->send(400, "text/plain", ERROR);
    }
    else
    {
        piscarResposta(SUCESSO);
        log("Parâmetros salvos");
        request->send(200, "text/plain", SUCESSO);
    }
}

void IRAM_ATTR contadorPulso()
{
    if (modoEnvase || modoCalibracao)
        qtdPulso++;
}

void calibracao(bool btnPressionado)
{
    if ((btnPressionado == false && modoCalibracao == true))
    {
        log("Encerrando modo calibração");
        PIN_CONTROL(PIN_VALVULA_ENVASE, TURN_OFF);
        PIN_CONTROL(PIN_MOTOR_BOMBA, TURN_OFF);
        qtdPulsoCalibracao = qtdPulso;
        // qtdPulso = 0;
        waitDelay(1000);
        PIN_CONTROL(PIN_VALVULA_LIBERACAO, TURN_OFF);
        PIN_CONTROL(PIN_LED_VERDE, TURN_OFF);
        log("Quantidade de pulsos: " + String(qtdPulsoCalibracao));
        modoCalibracao = false;
        return;
    }

    if (modoCalibracao == true)
        return; // se já estiver em modo de calibração, não fazer nada

    modoCalibracao = true;
    log("Modo calibração iniciado");
    qtdPulso = 0;
    qtdPulsoCalibracao = 0;
    qtdPulsoAMonitorar = 0;
    fatorCalibracao = 0.0;

    PIN_CONTROL(PIN_MOTOR_BOMBA, TURN_ON);
    PIN_CONTROL(PIN_VALVULA_ENVASE, TURN_ON);
    PIN_CONTROL(PIN_VALVULA_LIBERACAO, TURN_ON);
    PIN_CONTROL(PIN_LED_VERDE, TURN_ON);
    log("Calibrando...");
}

void envase()
{
    if (modoEnvase == true && (totalMilliLitres > volumeAserEnvasado || qtdPulso > qtdPulsoAMonitorar))
    {
        PIN_CONTROL(PIN_VALVULA_ENVASE, TURN_OFF);
        PIN_CONTROL(PIN_MOTOR_BOMBA, TURN_OFF);
        PIN_CONTROL(PIN_VALVULA_SELAGEM, TURN_ON);

        if (totalMilliLitres > volumeAserEnvasado)
            log("Volume (" + String(totalMilliLitres) + "mL) atingido");

        if (qtdPulso > qtdPulsoAMonitorar)
            log("Quantidade de pulsos (" + String(qtdPulso) + ") atingida");

        PIN_CONTROL(PIN_LED_VERDE, TURN_OFF);
        log("Modo envase encerrado");
        modoEnvase = false;
    }

    // se já estiver em modo de envase, não há nada a ser feito
    if (modoEnvase == true)
        return;

    if (fatorCalibracao == 0)
    {
        log("Fator de calibração não definido");
        return;
    }
    if (qtdPulsoAMonitorar == 0)
    {
        log("Quantidade de pulsos a monitorar não definida");
        return;
    }
    if (tempoSelagem == 0)
    {
        log("Tempo de selagem não definido");
        return;
    }
    if (volumeAserEnvasado == 0)
    {
        log("Volume a ser envasado não definido");
        return;
    }

    log("Processo de envase iniciado");
    modoEnvase = true;

    qtdPulso = 0;
    flowRate = 0.0;
    flowMilliLitres = 0;
    totalMilliLitres = 0;
    medicaoPreviousMillis = 0;

    PIN_CONTROL(PIN_VALVULA_LIBERACAO, TURN_ON);
    waitDelay(1000);
    PIN_CONTROL(PIN_MOTOR_BOMBA, TURN_ON);
    PIN_CONTROL(PIN_VALVULA_ENVASE, TURN_ON);
    PIN_CONTROL(PIN_LED_VERDE, TURN_ON);
    log("Envasando...");
}

void setup()
{
    Serial.begin(115200);
    delay(100);
    log("Starting...");

    pinMode(PIN_BOTAO_PROCESSO, INPUT_PULLUP);
    pinMode(PIN_BOTAO_CALIBRACAO, INPUT_PULLUP);
    pinMode(PIN_BOTAO_EMERGENCIAL, INPUT_PULLUP);
    pinMode(PIN_MEDIDOR_FLUXO, INPUT);

    pinMode(PIN_VALVULA_LIBERACAO, OUTPUT);
    pinMode(PIN_VALVULA_ENVASE, OUTPUT);
    pinMode(PIN_VALVULA_SELAGEM, OUTPUT);
    pinMode(PIN_MOTOR_BOMBA, OUTPUT);
    pinMode(PIN_LED_VERDE, OUTPUT);
    pinMode(PIN_LED_BRANCO, OUTPUT);

    PIN_CONTROL(PIN_VALVULA_LIBERACAO, TURN_OFF);
    PIN_CONTROL(PIN_VALVULA_ENVASE, TURN_OFF);
    PIN_CONTROL(PIN_VALVULA_SELAGEM, TURN_OFF);
    PIN_CONTROL(PIN_MOTOR_BOMBA, TURN_OFF);
    PIN_CONTROL(PIN_LED_VERDE, TURN_OFF);
    PIN_CONTROL(PIN_LED_BRANCO, TURN_OFF);

    log("testes de pinos..");
    for (int i = 0; i < pinSize; i++)
    {
        if (i == PIN_BOTAO_PROCESSO || i == PIN_BOTAO_CALIBRACAO || i == PIN_BOTAO_EMERGENCIAL || i == PIN_MEDIDOR_FLUXO)
            continue;

        if (pin_names[i] == "")
            continue;

        PIN_CONTROL(i, TURN_ON);
        waitDelay(250);
        PIN_CONTROL(i, TURN_OFF);
        waitDelay(250);
    }

    // Configurar interrupção para contar pulsos do medidor de fluxo
    attachInterrupt(digitalPinToInterrupt(PIN_MEDIDOR_FLUXO), contadorPulso, RISING);

    // Configurando endereço IP estático
    if (!WiFi.config(local_IP, gateway, subnet, INADDR_NONE, INADDR_NONE))
        log("Falha ao configurar endereço IP estático");

    WiFi.setHostname(hostname.c_str());
    WiFi.begin(ssid, password);
    log("Conectando à rede Wi-Fi: " + String(ssid));

    runner.addTask(TarefaPiscaVerde);
    runner.addTask(TarefaWifi);
    runner.addTask(TarefaCalibracao);
    runner.addTask(TarefaEnvase);
    runner.addTask(TarefaParadaEmergencial);
    runner.addTask(TarefaMedicao);

    TarefaPiscaVerde.enable();
    TarefaWifi.enable();
    TarefaCalibracao.enable();
    TarefaEnvase.enable();
    TarefaParadaEmergencial.enable();
    TarefaMedicao.enable();

    qtdPulso = 0;
    flowRate = 0.0;
    flowMilliLitres = 0;
    totalMilliLitres = 0;
    medicaoPreviousMillis = 0;

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/html", index_html, processor); });

    server.on("/parametros", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    String response = String(qtdPulsoCalibracao) + "|" + String(fatorCalibracao) + "|" + tempoSelagem + "|" + volumeAserEnvasado;
    request->send(200, "text/plain", response); });

    server.on("/medicao", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    String eventosString = eventos;
    eventos = "";
    String atividadeVigente = modoEnvase ? "ENVASE" : (modoCalibracao ? "CALIBRAÇÃO" : "--"); 
    String response = (
        atividadeVigente +
        "|" + String(flowMilliLitres) + 
        "|" + String(totalMilliLitres) + 
        "|" + String(qtdEnvaseRealizado) + 
        "|" + String(qtdPulso) + 
        "|" + String(qtdPulsoAMonitorar) + 
        "|" + eventosString
    );
    request->send(200, "text/plain", response); });

    server.on("/salvar", HTTP_POST, salvar);

    // server.on("/salvarMedicao", HTTP_POST, salvar);

    // server.on("/salvarFatorCalibracao", HTTP_POST, salvar);

    // server.on("/salvarParametros", HTTP_POST, salvar);

    server.begin();
}

void loop()
{
    runner.execute();
}