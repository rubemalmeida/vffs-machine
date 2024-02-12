#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <TaskScheduler.h>

// Variáveis dos pinos de entrada e saída e controle de tempo
#define PIN_BOTAO_EMERGENCIAL 26 // botão para parada emergencial
#define PIN_BOTAO_CALIBRACAO 25  // botão para calibração da máquina
#define PIN_BOTAO_PROCESSO 27    // botão para iniciar o processo de envase
#define PIN_LED_VERDE 4          // led indicativo para o processo em andamento
#define PIN_LED_BRANCO 5         // caso o led esteja aceso, indica que a máquina está ligada e conectada ao Wi-Fim,
                                 // e caso esteja piscando, indica que a máquina está ligada, porém, não está conectada ao Wi-Fi
#define PIN_VALVULA_LIBERACAO 18 // RL4IN1 - LIBERACAO - válvula pneumatica para controle do fluxo de ar
#define PIN_VALVULA_ENVASE 19    // RL4IN2 - ENVASE - válvula pneumatica para controle do fluxo do fluido a ser envasado
#define PIN_VALVULA_SELAGEM 21   // RL4IN3 - SELAGEM - válvula pneumatica para controle do pistão de selagem
#define PIN_MOTOR_BOMBA 22       // RL4IN4 - BOMBA - motor para controle transporte do fluido a ser envasado
#define PIN_MEDIDOR_FLUXO 23     // medidor de fluxo para controle do volume de fluido a ser envasado

#define TURN_ON 0
#define TURN_OFF 1

#define PIN_CONTROL(pinValue, state) pinControl(pinValue, state, __func__)
#define GET_PULSE_COUNT() obterQtdPulsos(__func__)

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
const String LIQ_AGUA = "agua";
const String LIQ_MARACUJA = "maracuja";
const String LIQ_MORANGO = "morango";
const String LIQ_ABACAXI = "abacaxi";
const String LIQ_MANGA = "manga";

String tipoLiquido = "";

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
unsigned long btnCalibTempoPrimeiroPressionado = 0;

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

volatile long qtdPulsoCalibracao = 0; // Contador de pulsos do medidor de fluxo usado na calibração
volatile long qtdPulsoPorSegundo = 0; // Contador de pulsos do medidor de fluxo por segundo
volatile long qtdPulsos = 0;          // Contador de pulsos do medidor de fluxo
int qtdPulsoAMonitorar = 0;           // Quantidade de pulsos a monitorar
long pulse1Sec = 0;
float fatorCalibracao = 0.0;  // Fator de calibracao do medidor de fluxo
float tempoSelagem = 0.0;     // Tempo de selagem do pistão em segundos
float volumeAserEnvasado = 0; // Volume a ser envasado
long medicaoPreviousMillis = 0;
int intervalMedicao = 1000;
float flowRate;
unsigned int flowMilliLitres;
unsigned int totalMilliLitres;

int repeticoesPiscaBranco = 0;
int repeticoesPiscaVerde = 0;
int intervalo500 = 500;
int intervalo1000 = 1000;
int intervaloPisca = 0;

String eventos = ""; // String para armazenar os eventos ocorridos

Scheduler runner; // Scheduler para gerenciar as tarefas

// Código html do web server
const char index_html[] PROGMEM = R"rawliteral(%HTML%)rawliteral";

void checkWifi();
void checkPiscaVerde();
void checkCalibracao();
void checkEnvase();
void checkSelagem();
void checkParadaEmergencial();
void checkMedicao();
void calibracao(bool btnPressionado);
void envase(bool btnPressionado);

Task TarefaWifi(500, TASK_FOREVER, &checkWifi);
Task TarefaPiscaVerde(500, TASK_FOREVER, &checkPiscaVerde);
Task TarefaCalibracao(100, TASK_FOREVER, &checkCalibracao);
Task TarefaEnvase(100, TASK_FOREVER, &checkEnvase);
Task TarefaSelagem(100, TASK_FOREVER, &checkSelagem);
Task TarefaParadaEmergencial(100, TASK_FOREVER, &checkParadaEmergencial);
Task TarefaMedicao(50, TASK_FOREVER, &checkMedicao);

void log(String evento, String type = "")
{
    if (type == "start")
        Serial.println("------------------------------");
    eventos += evento + ";";
    Serial.println(evento);
}

void pinControl(int pinValue, int state, const char *callingFunction)
{
    String stateName = state == TURN_ON ? "ON" : "OFF";
    String pinName = "PIN" + String(pinValue);
    if (pinValue >= 0 && pinValue < pinSize)
    {
        if (pin_names[pinValue] == "")
            pinName += "-NÃO DEFINIDO";
        // else if (pinValue == PIN_LED_BRANCO || pinValue == PIN_LED_VERDE)
        // return;
        else if (state == TURN_ON || state == TURN_OFF)
        {
            pinName = pin_names[pinValue];
            digitalWrite(pinValue, state);
            if (pinValue == PIN_LED_BRANCO)
                statusLedBranco = state == TURN_ON ? true : false;
            else if (pinValue == PIN_LED_VERDE)
                statusLedVerde = state == TURN_ON ? true : false;
        }
        else
            stateName += "ESTADO INVALIDO";
    }
    else
        pinName += "-DESCONHECIDO";

    log((String)callingFunction + " > " + stateName + ": " + pinName);
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

long obterQtdPulsos(const char *callingFunction)
{
    // Serial.println((String)callingFunction + " > Qtd de pulsos");
    // noInterrupts();
    long pulsos = qtdPulsos;
    // interrupts();
    return pulsos;
}

String processor(const String &var)
{
    if (var == "PULSOS-CALIB")
        return String(GET_PULSE_COUNT()) + "/" + String(qtdPulsoAMonitorar);

    // if (var == "FATOR-CALIBRACAO" && fatorCalibracao > 0)
    //     return String(fatorCalibracao);

    if (var == "TIPO-LIQUIDO")
        return tipoLiquido;

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

    if (var == "HIDDEN-FORM-TIPO-LIQUIDO") // && tipoLiquido != "")
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
    // Processo de calibração
    btnCalibEstadoCorrente = digitalRead(PIN_BOTAO_CALIBRACAO);
    if (
        btnCalibUltimoEstado == HIGH &&
        btnCalibEstadoCorrente == LOW &&
        paradaEmergencial == false &&
        (millis() - btnCalibTempoPrimeiroPressionado) >= 500)
    {
        btnCalibTempoPrimeiroPressionado = millis();
        if ((millis() - btnCalibTempoUltimoAcionamento) >= INTERVALO_MINIMO)
        {
            log("Botão de calibracao pressionado", "start");
            btnCalibTempoUltimoAcionamento = millis();
            btnCalibUltimoEstadoEfetivo = HIGH;
            calibracao(true);
        }
    }
    else if (
        (btnCalibUltimoEstado == LOW &&
         btnCalibEstadoCorrente == HIGH &&
         btnCalibUltimoEstadoEfetivo == HIGH) ||
        (modoCalibracao && paradaEmergencial))
    {
        log("Botão de calibracao liberado");
        btnCalibUltimoEstadoEfetivo = LOW;
        log("c1");
        btnCalibTempoPrimeiroPressionado = 0;
        log("c2");
        // calibracao(false);
        log("Encerrando modo calibracao");
        log("c2i.1");
        PIN_CONTROL(PIN_VALVULA_ENVASE, TURN_OFF);
        log("c2i.2");
        PIN_CONTROL(PIN_MOTOR_BOMBA, TURN_OFF);
        log("c2i.3");
        modoCalibracao = false;
        log("c2i.4");
        qtdPulsoCalibracao = GET_PULSE_COUNT();
        // waitDelay(1000);
        log("c2i.5");
        PIN_CONTROL(PIN_VALVULA_LIBERACAO, TURN_OFF);
        log("c2i.6");
        PIN_CONTROL(PIN_LED_VERDE, TURN_OFF);
        log("Quantidade de pulsos: " + String(qtdPulsoCalibracao));
        log("c3");
        if (paradaEmergencial)
            paradaEmergencial = false;
    }

    if (btnCalibUltimoEstado != btnCalibEstadoCorrente)
        btnCalibUltimoEstado = btnCalibEstadoCorrente;
}

void checkEnvase()
{
    // Processo de envase
    btnProcEstadoCorrente = digitalRead(PIN_BOTAO_PROCESSO);
    if (
        paradaEmergencial == false &&                     // Se a parada emergencial não estiver acionada
        !modoEnvase && !modoCalibracao && !modoSelagem && // Se não estiver em nenhum modo
        btnProcUltimoEstado == HIGH &&
        btnProcEstadoCorrente == LOW &&
        (millis() - btnProcStartAcionamento) >= 500)
    {
        btnProcStartAcionamento = millis();
        if ((millis() - btnProcTempoUltimoAcionamento) >= INTERVALO_MINIMO)
        {
            log("Botão de envase pressionado");
            btnProcTempoUltimoAcionamento = millis();
            btnProcUltimoEstadoEfetivo = HIGH;
            envase(true);
        }
    }
    else if ((btnProcUltimoEstado == LOW &&
              btnProcEstadoCorrente == HIGH &&
              btnProcUltimoEstadoEfetivo == HIGH) ||
             (modoEnvase && btnProcUltimoEstadoEfetivo == LOW) ||
             (modoEnvase && paradaEmergencial))
    {
        envase(false);
        btnProcUltimoEstadoEfetivo = LOW;
        if (paradaEmergencial)
            paradaEmergencial = false;
    }

    if (btnProcUltimoEstado != btnProcEstadoCorrente)
        btnProcUltimoEstado = btnProcEstadoCorrente;
}

void checkSelagem()
{
    if (modoSelagem)
    {
        if (inicioSelagem == 0)
        {
            inicioSelagem = millis();
            log("Selando... (" + String(tempoSelagem) + "s)");
            PIN_CONTROL(PIN_VALVULA_LIBERACAO, TURN_ON);
            PIN_CONTROL(PIN_VALVULA_SELAGEM, TURN_ON);
        }
        else if ((millis() - inicioSelagem >= tempoSelagem * 1000 || paradaEmergencial))
        {
            if (paradaEmergencial)
                paradaEmergencial = false;
            PIN_CONTROL(PIN_VALVULA_SELAGEM, TURN_OFF);
            waitDelay(1000);
            PIN_CONTROL(PIN_VALVULA_LIBERACAO, TURN_OFF);
            qtdEnvaseRealizado++;
            log("Selagem encerrada");
            modoSelagem = false;
        }
    }
}

void checkParadaEmergencial()
{
    if (!modoCalibracao && !modoEnvase && !modoSelagem)
        return;

    bool buttonState = digitalRead(PIN_BOTAO_EMERGENCIAL);
    if (buttonState == LOW)
    {
        if ((millis() - btnParadaStartAcionamento) > 100) // Se o botão foi mantido pressionado por 500ms
        {
            btnParadaStartAcionamento = millis(); // Registre o tempo de pressionamento do botão
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
}

void checkMedicao()
{
    if (((modoEnvase || modoCalibracao) && (millis() - medicaoPreviousMillis >= intervalMedicao)) ||
        (!modoEnvase && pulse1Sec > 0))
    {
        if ((modoEnvase && qtdPulsoAMonitorar > 0) || (!modoEnvase && pulse1Sec > 0))
        {
            long qtdPulsoAnterior = pulse1Sec;
            long qtdPulsoAtual = GET_PULSE_COUNT();
            if (qtdPulsoAtual == 0)
                return;
            pulse1Sec = qtdPulsoAtual - qtdPulsoAnterior;
            // flowRate = ((1000.0 / (millis() - medicaoPreviousMillis)) * pulse1Sec); // in L/min
            flowRate = 1;
            if (tipoLiquido == LIQ_AGUA)
                flowRate = 1.65E-03 * pulse1Sec + -2.55;

            if (!modoEnvase)
                pulse1Sec = 0;

            flowMilliLitres = flowRate;
            totalMilliLitres += flowRate;
            int tempoDecorrido = millis() - btnProcStartAcionamento;
            log("Envasando (" + String(tempoDecorrido / 1000) + "s)... " + String(qtdPulsoAtual) + " pulsos somados");
        }
        else if (modoCalibracao)
        {
            int tempoDecorrido = millis() - btnCalibTempoPrimeiroPressionado;
            log("Calibrando (" + String(tempoDecorrido / 1000) + "s)... " + String(GET_PULSE_COUNT()) + " pulsos somados");
        }

        medicaoPreviousMillis = millis();
    }
}

void salvar(AsyncWebServerRequest *request)
{
    int params = request->params();
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
        // else if (parametro == "input-medicao")
        // {
        //     if (qtdPulsoCalibracao > 0)
        //     {
        //         float volumeDispensado = valor.toFloat();
        //         log("Volume dispensado parametrizado: " + String(volumeDispensado));
        //         fatorCalibracao = (volumeDispensado / qtdPulsoCalibracao) * 1000000;
        //         log("Fator de calibracao calculado: " + String(fatorCalibracao));
        //     }
        //     else
        //     {
        //         log("Calibracao não identificada. Qtd de pulsos: " + String(qtdPulsoCalibracao));
        //         erro = true;
        //     }
        // }
        // else if (parametro == "input-fator-calibracao")
        // {
        //     fatorCalibracao = valor.toFloat();
        //     log("Fator de calibracao: " + String(fatorCalibracao));
        // }
        else if (parametro == "input-tempo-selagem")
            tempoSelagem = valor.toFloat();
        else if (parametro == "input-volume")
        {
            // if (fatorCalibracao > 0)
            // {
            qtdPulsoAMonitorar = 0;
            volumeAserEnvasado = valor.toFloat();
            if (tipoLiquido == LIQ_AGUA)
                qtdPulsoAMonitorar = 605 * volumeAserEnvasado + 1822;
            // qtdPulsoAMonitorar = round((volumeAserEnvasado * 1000000) / fatorCalibracao);
            log("Quantidade de pulsos a monitorar: " + String(qtdPulsoAMonitorar) + " (" + tipoLiquido + ")");
            // }
            // else
            // {
            //     log("Fator de calibracao pendente");
            //     erro = true;
            // }
        }
        else if (parametro == "input-tipo-liquido")
        {
            // if one of the valid types
            if (valor == LIQ_AGUA || valor == LIQ_MARACUJA || valor == LIQ_MORANGO || valor == LIQ_ABACAXI || valor == LIQ_MANGA)
            {
                tipoLiquido = valor;
                log("Tipo de líquido: " + tipoLiquido);
            }
            else
            {
                erro = true;
                log("Tipo de líquido não reconhecido: " + valor);
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
        qtdPulsos++;
}

void calibracao(bool btnPressionado)
{
    log("c2.1");
    if ((btnPressionado == false && modoCalibracao == true))
    {
        log("Encerrando modo calibracao");
        PIN_CONTROL(PIN_VALVULA_ENVASE, TURN_OFF);
        PIN_CONTROL(PIN_MOTOR_BOMBA, TURN_OFF);
        modoCalibracao = false;
        qtdPulsoCalibracao = GET_PULSE_COUNT();
        // waitDelay(1000);
        // PIN_CONTROL(PIN_VALVULA_LIBERACAO, TURN_OFF);
        PIN_CONTROL(PIN_LED_VERDE, TURN_OFF);
        log("Quantidade de pulsos: " + String(qtdPulsoCalibracao));
        return;
    }
    log("c2.2");
    if (modoCalibracao == true)
        return; // se já estiver em modo de calibração, não fazer nada

    log("c2.3");
    noInterrupts();
    qtdPulsos = 0;
    interrupts();

    log("c2.4");
    qtdPulsoCalibracao = 0;
    // qtdPulsoAMonitorar = 0;
    // fatorCalibracao = 0.0;

    PIN_CONTROL(PIN_VALVULA_LIBERACAO, TURN_ON);
    waitDelay(1000);

    modoCalibracao = true;
    log("Modo calibracao iniciado");

    PIN_CONTROL(PIN_MOTOR_BOMBA, TURN_ON);
    PIN_CONTROL(PIN_VALVULA_ENVASE, TURN_ON);
    PIN_CONTROL(PIN_LED_VERDE, TURN_ON);
    log("Calibrando...");
}

void envase(bool btnPressionado)
{
    if (modoEnvase == true)
    {
        long qtdPulsoAtual = GET_PULSE_COUNT();
        if (!btnPressionado && (totalMilliLitres > volumeAserEnvasado || qtdPulsoAtual >= qtdPulsoAMonitorar))
        {
            PIN_CONTROL(PIN_VALVULA_ENVASE, TURN_OFF);
            PIN_CONTROL(PIN_MOTOR_BOMBA, TURN_OFF);

            if (totalMilliLitres >= volumeAserEnvasado)
                log("Volume (" + String(totalMilliLitres) + "mL) atingido");

            if (qtdPulsoAtual >= qtdPulsoAMonitorar)
                log("Quantidade de pulsos (" + String(qtdPulsoAtual) + ") atingida");

            PIN_CONTROL(PIN_LED_VERDE, TURN_OFF);
            log("Modo envase encerrado");
            modoEnvase = false;
            modoSelagem = true;
        }
        return;
    }

    if (!btnPressionado)
        return;

    // if (fatorCalibracao == 0)
    // {
    //     log("Fator de calibracao não definido");
    //     return;
    // }
    if (tempoSelagem == 0 || volumeAserEnvasado == 0)
    {
        log("Tempo de selagem e/ou Volume a ser envasado não definidos");
        return;
    }
    if (qtdPulsoAMonitorar == 0)
    {
        log("Quantidade de pulsos a monitorar não definida");
        return;
    }

    noInterrupts();
    qtdPulsos = 0;
    interrupts();

    pulse1Sec = 0;
    flowRate = 0.0;
    flowMilliLitres = 0;
    totalMilliLitres = 0;
    medicaoPreviousMillis = 0;

    PIN_CONTROL(PIN_LED_VERDE, TURN_ON);
    PIN_CONTROL(PIN_VALVULA_LIBERACAO, TURN_ON);
    waitDelay(1000);

    modoEnvase = true;
    log("Processo de envase iniciado");

    PIN_CONTROL(PIN_MOTOR_BOMBA, TURN_ON);
    PIN_CONTROL(PIN_VALVULA_ENVASE, TURN_ON);
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
    pinMode(PIN_MEDIDOR_FLUXO, INPUT_PULLDOWN);

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

    // log("testes de pinos..");
    // for (int i = 0; i < pinSize; i++)
    // {
    //     if (i == PIN_BOTAO_PROCESSO || i == PIN_BOTAO_CALIBRACAO || i == PIN_BOTAO_EMERGENCIAL || i == PIN_MEDIDOR_FLUXO)
    //         continue;
    //     if (pin_names[i] == "")
    //         continue;
    //     PIN_CONTROL(i, TURN_ON);
    //     waitDelay(250);
    //     PIN_CONTROL(i, TURN_OFF);
    //     waitDelay(250);
    // }

    // Configurar interrupção para contar pulsos do medidor de fluxo
    attachInterrupt(digitalPinToInterrupt(PIN_MEDIDOR_FLUXO), contadorPulso, RISING);

    // Configurando endereço IP estático
    if (!WiFi.config(local_IP, gateway, subnet, INADDR_NONE, INADDR_NONE))
        log("Falha ao configurar endereço IP estático");

    WiFi.setHostname(hostname.c_str());
    WiFi.begin(ssid, password);
    log("Conectando à rede Wi-Fi: " + String(ssid));

    runner.addTask(TarefaWifi);
    runner.addTask(TarefaPiscaVerde);
    runner.addTask(TarefaCalibracao);
    runner.addTask(TarefaEnvase);
    runner.addTask(TarefaSelagem);
    runner.addTask(TarefaParadaEmergencial);
    runner.addTask(TarefaMedicao);

    TarefaPiscaVerde.enable();
    TarefaWifi.enable();
    TarefaCalibracao.enable();
    TarefaEnvase.enable();
    TarefaSelagem.enable();
    TarefaParadaEmergencial.enable();
    TarefaMedicao.enable();

    flowRate = 0.0;
    flowMilliLitres = 0;
    totalMilliLitres = 0;
    medicaoPreviousMillis = 0;

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/html", index_html, processor); });

    server.on("/parametros", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    String response = String(qtdPulsoCalibracao) + "|" + tipoLiquido + "|" + tempoSelagem + "|" + volumeAserEnvasado;
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
        "|" + String(GET_PULSE_COUNT()) + 
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
