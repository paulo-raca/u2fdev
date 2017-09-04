/**
\file
Funções de baixo nível (Sem interface gráfica) para adicionar biometria em aplicações.

As funções incluem as funcionalidade de captura, extração e comparação biométrica.

Consulte a documentação da API: \ref veridisbiometric.h
*/

#ifndef VERIDIS_BIOMETRIC_H
#define VERIDIS_BIOMETRIC_H

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Enumeração dos tipos de biometria.
 * @see VrBio_BiometricImage
 */
typedef enum {
  /**Tipo desconhecido*/
  VRBIO_BIOMETRIC_MODALITY_UNKNOWN,
  /**Impressão digital, impressão palmar e afins*/
  VRBIO_BIOMETRIC_MODALITY_FINGERPRINT,
  /**Fotografia da face */
  VRBIO_BIOMETRIC_MODALITY_FACE,              
  /**Padrões vasculares, estilo PalmVein*/
  VRBIO_BIOMETRIC_MODALITY_VEINS,              
  /**Fotogriafia da irs */
  VRBIO_BIOMETRIC_MODALITY_IRIS               
} VrBio_BiometricModality;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Enumeração dos tipos de leitores biométricos.
 * @see VrBio_BiometricImage
 */
typedef enum {
  /**Leitor biométrico desconhecido*/
  VRBIO_SCANNER_TYPE_UNKNOWN,
  /**Leitor biométrico tipo foto*/
  VRBIO_SCANNER_TYPE_PHOTO,
  /**Leitor biométrico tipo rolado*/
  VRBIO_SCANNER_TYPE_ROLLED,
  /**Leitor biométrico tipo arrasto (igual dos notebooks) */
  VRBIO_SCANNER_TYPE_SWIPE
} VrBio_ScannerType;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Enumeração dos eventos de captura lançados pela biblioteca de captura.
 * @see VrBio_CaptureEventCallback
 */
typedef enum {
  /**Leitor removido*/
  VRBIO_CAPTURE_EVENT_UNPLUG                      =0x001,
  /**Leitor adicionado*/
  VRBIO_CAPTURE_EVENT_PLUG                        =0x002,
  /**Dedo foi retirado do leitor*/
  VRBIO_CAPTURE_EVENT_REMOVED                     =0x004,
  /**Dedo foi colocado sobre o leitor*/
  VRBIO_CAPTURE_EVENT_PLACED                      =0x008,
  /**Frame recebido. Evento totalmente inútil, mas pode ser user-friendly*/
  VRBIO_CAPTURE_EVENT_IMAGE_FRAME                 =0x10,
  /**Imagem capturada*/
  VRBIO_CAPTURE_EVENT_IMAGE_CAPTURED              =0x020,
  /**Pede para o usuário remover o dedo*/
  VRBIO_CAPTURE_EVENT_USER_REMOVE                 =0x040,
  /**All plugged devices have been enumerated. Other Plug/Unplug events will come from hotplugging */
  VRBIO_CAPTURE_EVENT_PLUG_FINISHED               =0x080,
  /** A sun reflection has been detected on the sensor */
  VRBIO_CAPTURE_EVENT_SUN_REFLECTION_DETECTED     =0x100,
  /** A sun reflection has been removed from the sensor */
  VRBIO_CAPTURE_EVENT_SUN_REFLECTION_REMOVED      =0x200,
  /** A false finger has been detected on the sensor */
  VRBIO_CAPTURE_EVENT_FAKE_FINGER_DETECTED        =0x400,
  /** A false finger has been removed from the sensor */
  VRBIO_CAPTURE_EVENT_FAKE_FINGER_REMOVED         =0x800
} VrBio_EventType;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**Enumeração dos códigos de erro da biblioteca.*/
typedef enum {
  /**Função retornou com sucesso*/
  VRBIO_SUCCESS                                     = 0,
  /**Erro interno*/
  VRBIO_ERROR_FAIL                                  =-1,
  /**Argumento inválido passado para a função*/
  VRBIO_ERROR_ARGUMENT                              =-2,
  /**veridiscap_addListener ou veridiscap_addListenerToReader chamado várias vezes*/
  VRBIO_ERROR_ALREADY_STARTED                       =-3,
  /**veridiscap_removeListener/veridiscap_removeListenerFromReader sem veridiscap_addListener/veridiscap_addListenerToReader anterior.*/
  VRBIO_ERROR_NOT_STARTED                           =-4,
  /**Tempo máximo da captura síncrona excedido*/
  VRBIO_ERROR_TIMEOUT                               =-5,
  /**Leitor Biométrico já está conectado*/
  VRBIO_ERROR_ALREADY_PLUGGED                       =-6,
  /**Leitor Biométrico não está conectado*/
  VRBIO_ERROR_NOT_PLUGGED                           =-7,
  /**Operação não suportada*/
  VRBIO_ERROR_UNSUPPORTED_OPERATION                 =-8,
  /**Formato especificado não é suportado*/
  VRBIO_ERROR_UNSUPPORTED_FORMAT                    =-9,
  /**Tipo de biometria especificada é inválido*/
  VRBIO_ERROR_UNSUPPORTED_BIOMETRIC_MODALITY        =-10,
  /**Template é inválido*/
  VRBIO_ERROR_INVALID_TEMPLATE                      =-11,
  /**Licença ainda não foi instalada com \ref veridisutil_installLicense*/
  VRBIO_ERROR_NOT_LICENSED                          =-12,
  /**A funcionalidade não está disponível para a licença atual */
  VRBIO_ERROR_FEATURE_NOT_LICENSED                  =-13,
  /**Licença Inválida - Product Key inexistente ou Arquivo de licença corrompido */
  VRBIO_ERROR_INVALID_LICENSE                       =-14,
  /**Licença está expirada, ou não é válida para este hardware */
  VRBIO_ERROR_LICENSE_EXPIRED                       =-15,
  /**Licença foi ativada em mais clientes do que o permitido*/
  VRBIO_ERROR_LICENSE_USERS_EXCEEDED                =-16,
  /**Não foi possível abrir uma conexão ao servidor de licenças / Proxy */
  VRBIO_ERROR_HTTP_CONNECTION_FAIL                  =-17,
  /**Funcionalidade não disponível na versão não-licenciada (FREE) do SDK */
  VRBIO_ERROR_NOT_AVAILABLE_ON_FREE_EDITION         =-18
} VrBio_ErrorCodes;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Enumeração das propriedades dos leitores biométricos.
 * @see veridiscap_getReaderString
 */
typedef enum {
  /**Nome simpático do leitor. Suportado em todos os leitores.*/
  VRBIO_CAPTURE_READER_FRIENDLY_NAME,
  /**Nome do fabricante do leitor. Suportado em todos os leitores.*/
  VRBIO_CAPTURE_READER_VENDOR,
  /**Nome oficial do leitor. Suportado em todos os leitores.*/
  VRBIO_CAPTURE_READER_PRODUCT,
  /**Número de série do leitor. Suportado apenas em alguns leitores.*/
  VRBIO_CAPTURE_READER_SERIAL,
  /**Porta onde o dispositivo está conectado. O formato depende do sistema, mas pode ser usb1-1-3 */
  VRBIO_CAPTURE_READER_ADDRESS,
  /** Version of the Device (May include Hardware, Firmware and Software) */
  VRBIO_CAPTURE_READER_VERSION,
  /** Version of the Device's hardware */
  VRBIO_CAPTURE_READER_HARDWARE_VERSION,
  /** Version of the Device's firmware */
  VRBIO_CAPTURE_READER_FIRMWARE_VERSION,
  /** Version of the Device's software */
  VRBIO_CAPTURE_READER_SOFTWARE_VERSION,
} VrBio_ReaderProperty;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Enumeração das propriedades da licença.
 * @see veridisutil_getLicenseTextEx
 */
typedef enum {
  /**Texto completo da licença. Pode ser utilizado para inicializar a biblioteca offline em alguns casos. */
  VRBIO_LICENSE_FULL_TEXT,
  /**Chave da licença. Pode ser utilizado para inicializar a biblioteca online. */
  VRBIO_LICENSE_KEY,
  /**Texto humanamente legível da licença. */
  VRBIO_LICENSE_TEXT,
  /**Nome do usuário responsável pela licença. */
  VRBIO_LICENSE_USER,
} VrBio_LicenseProperty;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Representa uma imagem biométrica: Impressão digital, fotografia facial, etc.
 * @see VrBio_CaptureEventCallback
 * @see veridiscap_synchronousCapture
 */
typedef struct {
  /**Largura desta imagem*/
  int width;
  /**Altura desta imagem*/
  int height;
  /**Resolução da imagem, em pontos por cm. Valor típico é 200pontos/cm */
  int resolution;
  /**Número de canais de cor desta image. 1 para imagens preto e branco, 3 para imagens coloridas.*/
  int channels;
  /**Tipo de biometria: Fingerprint, face, etc. Use os valores de \ref VrBio_BiometricModality.*/
  int biometricModality;
  /**Tipo do scanner: Rolado, arraste, foto, etc. Use os valores de \ref VrBio_ScannerType.*/
  int scannerType;
  /**Buffer contendo os pixels desta imagem.
    A posição do um pixel(x,y,c) é dada por y*width*channels+x*channels+c.
  */
  unsigned char* buffer;
} VrBio_BiometricImage;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Representa uma requisição para o servidor de licenças.
 * @see veridisutil_makeLicenseRequest
 */
typedef struct {
  /** License Server's host name'. Tipically "license.veridisbiometrics.com". */
  const char* host;
  /** License Server's port'. Tipically 80 (HTTP). */
  int   port;
  /** HTTP path. Tipically "/installLicense". */
  const char* path;
  /** Request body. This must be sent to the server as POST data. */
  const char* body;
} LicenseRequest;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Armazena os metadados de um leitor.
 * @see veridiscap_getReaderString
 * @see VrBio_ReaderProperty
 */
typedef struct {
  /** Identificador único do leitor. Presente em todos os leitores. */
  const char* uniqueName;
  
  /**
   * Nome simpático do leitor. Suportado em todos os leitores.
   * @see VRBIO_CAPTURE_READER_FRIENDLY_NAME
   */
  const char* friendlyName;
  
  /**
   * Nome do fabricante do leitor. Suportado em todos os leitores.
   * @see VRBIO_CAPTURE_READER_VENDOR
   */
  const char* vendorName;
  
  /**
   * @see VRBIO_CAPTURE_READER_PRODUCT
   * Nome oficial do leitor. Suportado em todos os leitores.
   */
  const char* productName;
  
  /**
   * Número de série do leitor. Será NULL se o leitor não suportar número de série.
   * @see VRBIO_CAPTURE_READER_SERIAL
   */
  const char* serial;
} VrBio_ReaderProperties;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
  Função de callback que irá receber os eventos da captura: Leitor conectado, imagem capturada, etc, etc.

  Os parâmetros readerName e image são válido apenas durante esta chamada do callback. 
  Não mantenha referencias a eles após o fim da chamada!
  
  @param[in]  eventType OR dos eventos recebidos. Por exemplo, para ver se houver um plug, teste (eventType & VrBio_EventType::VRBIO_CAPTURE_EVENT_PLUG).
  @param[in]  image Imagem correspondente a esse evento. Será não-nula para os eventos VrBio_EventType::VRBIO_CAPTURE_EVENT_IMAGE_FRAME e VrBio_EventType::VRBIO_CAPTURE_EVENT_IMAGE_CAPTURED.
  @param[in]  userData Parâmetro especificado no \ref veridiscap_addListener.
*/
typedef void (*VrBio_CaptureEventCallback) (
                   int  eventType, 
            const char* readerName, 
  VrBio_BiometricImage* image, 
            const void* userData);


//@}####################################################################################################################################
/// @name Captura Biométrica
//@{####################################################################################################################################


/**
  Começa a receber eventos de captura no listenerHandle especificado. 
  Inicialmente são recebidos apenas os eventos VrBio_EventType::VRBIO_CAPTURE_EVENT_PLUG e VrBio_EventType::VRBIO_CAPTURE_EVENT_UNPLUG. 
  Os leitores serão inicializados e começarão a gerar eventos ao chamar \ref veridiscap_addListenerToReader com este listenerHandle. 

  @param[in]  listenerHandle Identificador único do receptor de eventos, será passado nas chamadas de callback.
  @param[in]  eventCallback Função de callback para os eventos.
  @return Código de erro.
    - ErrorCodes::VRBIO_SUCCESS: OK.
    - ErrorCodes::VRBIO_ERROR_ARGUMENT: listenerHandle==NULL or eventCallback==NULL.
    - ErrorCodes::VRBIO_ERROR_ALREADY_STARTED: listenerHandle is already registered to receive events.
*/
int veridiscap_addListener   (
                  const void* listenerHandle, 
  VrBio_CaptureEventCallback  eventCallback);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Pára de receber eventos no listenerHandle especificado. Essa chamada anula as chamadas anteriores de \ref veridiscap_addListener e e \ref veridiscap_addListenerToReader.

  @param[in]  listenerHandle listenerHandle que irá parar de receber eventos.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: listenerHandle==NULL
    - VRBIO_ERROR_NOT_STARTED: listenerHandle is not registered to receive events
*/
int veridiscap_removeListener(const void* listenerHandle);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Inicializa o leitor especificado, passando a receber seus eventos no listenerHandle.
  É necessário executar \ref veridiscap_addListener com esse listenerHandle antes de chamar esta função!

  @param[in]  listenerHandle Identificado do objeto que irá receber os eventos.
  @param[in]  readerName Leitor do qual eu quero receber eventos.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: listenerHandle==NULL
    - VRBIO_ERROR_NOT_STARTED: listenerHandle não tá registrado para receber eventos
    - VRBIO_ERROR_NOT_PLUGGED: Dispositivo inexistente
    - VRBIO_ERROR_ALREADY_STARTED: listenerHandle já está recebendo eventos deste leitor
    - VRBIO_ERROR_FAIL: Se ocorrer um erro ao inicializar o leitor
    - VRBIO_ERROR_FEATURE_NOT_LICENSED: Licença não permite o uso deste leitor
    - VRBIO_ERROR_NOT_AVAILABLE_ON_FREE_EDITION: Se está tentando usar mais de um leitor biométrico simulaneamente.
*/
int veridiscap_addListenerToReader(
  const void* listenerHandle, 
  const char* readerName);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Inicializa o leitor especificado, passando a receber seus eventos no listenerHandle.
  É necessário executar \ref veridiscap_addListener com esse listenerHandle antes de chamar esta função!

  @param[in]  listenerHandle Identificado do objeto que irá parar de receber eventos.
  @param[in]  readerName Leitor do qual eu quero parar de receber eventos.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: listenerHandle==NULL
    - VRBIO_ERROR_NOT_STARTED: listenerHandle não tá registrado para receber eventos deste leitor
    - VRBIO_ERROR_NOT_PLUGGED: Dispositivo inexistente
*/
int veridiscap_removeListenerFromReader(
  const void* listenerHandle, 
  const char* readerName);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Consulta uma propriedade do leitor especificado. 
  
  As propriedades suportadas atualmente são:
    - VrBio_ReaderProperty::VRBIO_CAPTURE_READER_FRIENDLY_NAME
    - VrBio_ReaderProperty::VRBIO_CAPTURE_READER_VENDOR
    - VrBio_ReaderProperty::VRBIO_CAPTURE_READER_PRODUCT
    - VrBio_ReaderProperty::VRBIO_CAPTURE_READER_SERIAL

  Não esqueça de chamar \ref veridisutil_stringFree depois!

  @param[in]  readerName Identificador do leitor consultado.
  @param[in]  code Qual a propriedade consultada.
  @param[out] value Variável onde a string será armazenada.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_NOT_PLUGGED: Dispositivo inexistente
    - VRBIO_ERROR_UNSUPORTED_OPERATION: Propriedade indisponível neste leitor
*/
int veridiscap_getReaderString(
  const char*  readerName,
         int   code, 
  const char** value);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Inicializa o sistema de captura e aguarda que uma imagem seja capturada.
  Faz tudo isso de forma síncrona, isto é, essa thread ficará suspensa até que a imagem seja obtida, ou que o tempo limite seja atingido.

  Ainda não há qualquer tratamento de "Nenhum leitor conectado" ou forma de cancelar a captura depois da chamada, portanto use com cuidado.

  A imagem resultante deverá ser liberada com \ref veridisutil_imageFree e o nome do leitor utilizado deverá ser liberado com \ref veridisutil_stringFree.

  @param[in]  readerName A captura só será realizada com leitores que tiverem esse valor como sub-string do seu nome. No caso genérico, NULL ou "" permitem que a captura seja realizada com qualquer leitor.
  @param[in]  timeout Tempo limite (Em millisegundos) para captura da imagem. Depois disso, a função retorna \ref VRBIO_ERROR_TIMEOUT. Use timeout=0 para desabilitiar o timeout.
  @param[out] out_image Variável que irá armazenar a imagem capturada. NULL para descartar a imagem.
  @param[out] actualReaderName Variável que irá armazenar o nome do leitor utilizado na captura. NULL para descartar o nome.
  @return Código de erro.
    - VRBIO_SUCCESS: OK.
    - VRBIO_ERROR_TIMEOUT: Nenhuma captura foi feita dentro do timeout especificado.
    - VRBIO_ERROR_FEATURE_NOT_LICENSED: A licença isntalada não permite o uso da captura.
*/
int veridiscap_synchronousCapture(
            const char*  readerName, 
                   int   timeout, 
  VrBio_BiometricImage** out_image, 
            const char** actualReaderName);

/**
 * Lista os nomes de todos os leitores conectados.
 * 
 * Não esqueça de liberar o vetor lido com \ref veridisutil_stringListFree.
 * 
 * Exemplo de uso:
 * \code
void listDevices() {
  int N;
  char** deviceList = NULL;
  int ret=veridiscap_listDevices(&deviceList, &N);
  if (ret < 0)  {
    printf("listDevices falhou. Erro=%i\n", ret);
    return;
  }
  printf("Device List: %i items\n", N);
  for (int i=0; i<N; i++) {
    printf("  - %s\n", deviceListEx[i]);
  }
  veridisutil_stringListFree(&deviceList);
}
 * \endcode
 * 
 * @param[out] array pointer to the variable that will contain the array of strings with the name of the scanners.
 * @param[out] N pointer to the variable that will contain the number of elements in \a array.
 */
int veridiscap_listDevices(const char*** array, int* N);

/**
 * Lista as propriedades (Modelo, fabricante, número de serie, etc) de todos os leitores conectados.
 * 
 * Não esqueça de liberar o vetor lido com \ref veridisutil_ReaderPropertiesListFree.
 * 
 * \see VrBio_ReaderProperties
 * \see veridiscap_getReaderString
 * 
 * Exemplo de uso:
 * \code
void listDevicesEx() {
  int N;
  VrBio_ReaderProperties** deviceListEx = NULL;
  int ret=veridiscap_listDevicesEx(&deviceListEx, &N);
  if (ret < 0)  {
    printf("listDevicesEx falhou. Erro=%i\n", ret);
    return;
  }
  printf("Device List: %i items\n", N);
  for (int i=0; i<N; i++) {
    printf(
	"  - %s/%s/%s/%s/%s\n", 
	deviceListEx[i]->uniqueName, 
	deviceListEx[i]->friendlyName, 
	deviceListEx[i]->vendorName, 
	deviceListEx[i]->productName, 
	deviceListEx[i]->serial );
  }
  veridisutil_ReaderPropertiesListFree(&deviceListEx);
}
 * \endcode
 * 
 * @param[out] array pointer to the variable that will contain the array of strings with the name of the scanners.
 * @param[out] N pointer to the variable that will contain the number of elements in \a array.
 */
int veridiscap_listDevicesEx(VrBio_ReaderProperties*** array, int* N);

//@}####################################################################################################################################
///@name Extração de imagens
//@{####################################################################################################################################


/**
  Extrai o template da imagem, no formato padrão (O formato padrão poderá depender do tipo de biometria, configurações, versão, licença, etc)
  
  Não esqueça de liberar o template resultante com \ref veridisutil_templateFree.
  
  Na verdade, este método é apenas uma forma abreviada de \code veridisbio_extractEx(img, tpt_buffer, tpt_size, NULL, NULL) \endcode

  @param[in]  img Imagem que será processada.
  @param[out] tpt_buffer Irá apontar para o template criado.
  @param[out] tpt_size Irá conter o tamanho do template criado. Pode ser NULL.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: img==NULL ou img->buffer==NULL ou tpt_buffer==NULL
    - VRBIO_ERROR_FAIL: Impossível realizar a extração.
    - VRBIO_ERROR_UNSUPPORTED_BIOMETRIC_MODALITY: O tipo de biometria (img->biometricModality) não é suportado.
    - VRBIO_ERROR_UNSUPPORTED_FORMAT: Se a configuração com o formato de templates padrão for inválida.
    - VRBIO_ERROR_INVALID_TEMPLATE: O formato de templates escolhido não é compativel com o template criado. Provavelmente a biometria não é compatível com o tipo do template.
    - VRBIO_ERROR_FEATURE_NOT_LICENSED: Licença não permite a extração deste tipo de biometria ou tipo de template.
    - VRBIO_ERROR_NOT_AVAILABLE_ON_FREE_EDITION: Tentando extrair uma imagem que não foi obtida pelo módulo de captura.
*/
int veridisbio_extract(
  const VrBio_BiometricImage*  img, 
                        char** tpt_buffer, 
                         int*  tpt_size);

  
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Extrai o template da imagem, no formato especificado. Caso o formato seja nulo, o formato padrão será utilizado (O formato padrão poderá depender to tipo de biometria, configurações, versão, licença, etc)

  Não esqueça de liberar o template resultante com \ref veridisutil_templateFree.

  Adicionalmente, o nome "canônico" do formato utilizado será retornado em out_tpt_format. Este nome deverá ser liberado através de \ref veridisutil_stringFree.

  @see template_formats
  @param[in]  img Imagem que será processada.
  @param[out] tpt_buffer Irá apontar para o template criado.
  @param[out] tpt_size Irá conter o tamanho do template criado. Pode ser NULL.
  @param[in]  in_tpt_format Formato no qual o template deverá ser serializado. NULL para usar o formato padrão.
  @param[out] out_tpt_format Nome "canônico" do formato no qual o template deverá ser serializado. NULL descartá-lo.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: img==NULL ou img->buffer==NULL ou tpt_buffer==NULL
    - VRBIO_ERROR_FAIL: Impossível realizar a extração.
    - VRBIO_ERROR_UNSUPPORTED_BIOMETRIC_MODALITY: O tipo de biometria (img->biometricModality) não é suportado.
    - VRBIO_ERROR_UNSUPPORTED_FORMAT: Se a configuração com o formato de templates padrão for inválida.
    - VRBIO_ERROR_INVALID_TEMPLATE: O formato de templates escolhido não é compativel com o template criado. Provavelmente a biometria não é compatível com o tipo do template.
    - VRBIO_ERROR_FEATURE_NOT_LICENSED: Licença não permite a extração deste tipo de biometria ou tipo de template.
    - VRBIO_ERROR_NOT_AVAILABLE_ON_FREE_EDITION: Tentando extrair uma imagem que não foi obtida pelo módulo de captura.
*/
int veridisbio_extractEx(
  const VrBio_BiometricImage*  img,
                        char** tpt_buffer, 
                         int*  tpt_size,
                  const char*  in_tpt_format, 
                  const char** out_tpt_format);

  
//@}####################################################################################################################################
///@name Verificação de templates
//@{####################################################################################################################################


/**
  Realiza o matching entre os templates especificados, retornando uma pontuação que representa o nível de similaridade entre eles.
  
  Na verdade, este método é apenas uma forma abreviada de \code veridisbio_matchEx (tpt1_buffer, tpt1_size, NULL, tpt2_buffer, tpt2_size, NULL) \endcode.
  
  @param[in]  tpt1_buffer Buffer contendo o primeiro template.
  @param[in]  tpt1_size   Tamanho do buffer contendo o primeiro template.
  @param[in]  tpt2_buffer Buffer contendo o segundo template.
  @param[in]  tpt2_size   Tamanho do buffer contendo o segundotemplate.
  @return A similaridade entre os templates, ou o código de erro se <0.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: tpt1==NULL ou tpt2==NULL ou tpt1size<0 ou tpt2size<0
    - VRBIO_ERROR_UNSUPPORTED_FORMAT: Um dos templates está em formato não suportado/não reconhecido
    - VRBIO_ERROR_INVALID_TEMPLATE: Um dos templates é inválido
    - VRBIO_ERROR_FAIL: Erro interno ao realizar o matching
    - VRBIO_ERROR_FEATURE_NOT_LICENSED: Licença não permite matching deste tipo de biometria ou deste tipo de template.
    - VRBIO_ERROR_NOT_AVAILABLE_ON_FREE_EDITION: Tentando comparar templates de leitores biométricos diferentes, ou dos quais não seja possível determinar o leitor biométrico.
*/
int veridisbio_match(
  const char* tpt1_buffer,
         int  tpt1_size,
  const char* tpt2_buffer, 
         int  tpt2_size);

  
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Realiza o matching entre os templates especificados, retornando uma pontuação que representa o nível de similaridade entre eles.
  
  O formato do template deve ser usado para aqueles formatos que não podem ser reconhecidos automaticamente. Na maioria dos casos, é suficiente deixá-los me branco (null).
  
  @see template_formats
  @param[in]  tpt1_buffer Buffer contendo o primeiro template.
  @param[in]  tpt1_size   Tamanho do buffer contendo o primeiro template.
  @param[in]  tpt1_format Formato do primeiro template. Pode ser NULL para detectar o formato automaticamente.
  @param[in]  tpt2_buffer Buffer contendo o segundo template.
  @param[in]  tpt2_size   Tamanho do buffer contendo o segundotemplate.
  @param[in]  tpt2_format Formato do segundo template. Pode ser NULL para detectar o formato automaticamente.
  @return A similaridade entre os templates, ou o código de erro se <0.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: tpt1==NULL ou tpt2==NULL ou tpt1size<0 ou tpt2size<0
    - VRBIO_ERROR_UNSUPPORTED_FORMAT: Um dos templates está em formato não suportado ou não reconhecido.
    - VRBIO_ERROR_INVALID_TEMPLATE: Um dos templates é inválido.
    - VRBIO_ERROR_FAIL: Erro interno ao realizar o matching.
    - VRBIO_ERROR_FEATURE_NOT_LICENSED: Licença não permite matching deste tipo de biometria ou deste tipo de template.
    - VRBIO_ERROR_NOT_AVAILABLE_ON_FREE_EDITION: Tentando comparar templates de leitores biométricos diferentes, ou dos quais não seja possível determinar o leitor biométrico.
*/
int veridisbio_matchEx(
  const char* tpt1_buffer, 
         int tpt1_size, 
  const char* tpt1_format,
  const char* tpt2_buffer, 
         int tpt2_size, 
  const char* tpt2_format);


//@}####################################################################################################################################
///@name Identificação de templates
//@{####################################################################################################################################


/**
  Inicia uma identificação biométrica 1-para-N.
  
  Este método irá receber o template sendo buscado (<i>candidato</i>) e irá criar um <i>Contexto de Identificação</i> capaz de fazer matching rapidamente.
  
  Para cada um dos templates sendo testados é preciso usar \ref veridisbio_identify.
  
  Quando a busca tiver terminado, é preciso usar \ref veridisbio_terminateIdentification para liberar os recursos alocados.
  
  @param[out] context     Variável que irá armazenar o <i>Contexto de Identificação</i> criado.
  @param[in]  tpt_buffer  Buffer contendo o template candidato.
  @param[in]  tpt_size    Tamanho do buffer contendo o template candidato.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: tpt_buffer==NULL ou context==NULL ou tpt_size<0
    - VRBIO_ERROR_UNSUPPORTED_FORMAT: O template está em formato não suportado ou não reconhecido.
    - VRBIO_ERROR_INVALID_TEMPLATE: O template é inválido.
    - VRBIO_ERROR_FAIL: Erro interno ao realizar o matching.
    - VRBIO_ERROR_FEATURE_NOT_LICENSED: Licença não permite identificação ou não permite este tipo de template.
*/
int veridisbio_prepareIdentification(
        void** context, 
  const char*  tpt_buffer, 
         int   tpt_size);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Inicia uma identificação biométrica 1-para-N.
  
  Este método irá receber o template sendo buscado (<i>candidato</i>) e irá criar um <i>Contexto de Identificação</i> capaz de fazer matching rapidamente.
  
  Para cada um dos templates sendo testados é preciso usar \ref veridisbio_identify.
  
  Quando a busca tiver terminado, é preciso usar \ref veridisbio_terminateIdentification para liberar os recursos alocados.
  
  @see template_formats
  @param[out] context     Variável que irá armazenar o <i>Contexto de Identificação</i> criado.
  @param[in]  tpt_buffer  Buffer contendo o template candidato.
  @param[in]  tpt_size    Tamanho do buffer contendo o template candidato.
  @param[in]  tpt_format  Formato do template candidato. Pode ser NULL para detectar o formato automaticamente.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: tpt_buffer==NULL ou context==NULL ou tpt_size<0
    - VRBIO_ERROR_UNSUPPORTED_FORMAT: O template está em formato não suportado ou não reconhecido.
    - VRBIO_ERROR_INVALID_TEMPLATE: O template é inválido.
    - VRBIO_ERROR_FAIL: Erro interno ao realizar o matching.
    - VRBIO_ERROR_FEATURE_NOT_LICENSED: Licença não permite identificação ou não permite este tipo de template.
*/
int veridisbio_prepareIdentificationEx(
        void** context, 
  const char*  tpt_buffer, 
         int   tpt_size, 
  const char*  tpt_format);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
  Realiza o matching entre o template sendo identificado e o template especificado por parâmetro, retornando uma pontuação que representa o nível de similaridade entre eles.
  
  Na verdade, este método é apenas uma forma abreviada de \code veridisbio_identifyEx(context, tpt_buffer, tpt_size, NULL)\endcode.

  @param[in]  context Contexto da Identificação, criado por \ref veridisbio_prepareIdentification. Contém o primeiro template do matching.
  @param[in]  tpt_buffer Buffer contendo o segundo template do matching.
  @param[in]  tpt_size   Tamanho do buffer contendo o segundo template.
  @return A similaridade entre os templates, ou o código de erro se <0.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: context==NULL ou tpt_buffer==NULL tpt_size<0
    - VRBIO_ERROR_UNSUPPORTED_FORMAT: O template está em formato não suportado ou não reconhecido.
    - VRBIO_ERROR_INVALID_TEMPLATE: O template é inválido.
    - VRBIO_ERROR_FAIL: Erro interno ao realizar o matching.
    - VRBIO_ERROR_FEATURE_NOT_LICENSED: Licença não permite este tipo de template.
    - VRBIO_ERROR_NOT_AVAILABLE_ON_FREE_EDITION: Tentando comparar templates de leitores biométricos diferentes, ou dos quais não seja possível determinar o leitor biométrico.
*/
int veridisbio_identify(
        void* context,
  const char* tpt_buffer, 
         int  tpt_size);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Realiza o matching entre o template sendo identificado e o template especificado por parâmetro, retornando uma pontuação que representa o nível de similaridade entre eles.
  
  O formato do template deve ser usado para aqueles formatos que não podem ser reconhecidos automaticamente. Na maioria dos casos, é suficiente deixá-los me branco (null).
  
  @see template_formats
  @param[in]  context Contexto da Identificação, criado por \ref veridisbio_prepareIdentification. Contém o primeiro template do matching.
  @param[in]  tpt_buffer Buffer contendo o segundo template do matching.
  @param[in]  tpt_size   Tamanho do buffer contendo o segundo template.
  @param[in]  tpt_format Formato do segundo template. Pode ser NULL para detectar o formato automaticamente.
  @return A similaridade entre os templates, ou o código de erro se <0.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: context==NULL ou tpt_buffer==NULL tpt_size<0
    - VRBIO_ERROR_UNSUPPORTED_FORMAT: O template está em formato não suportado ou não reconhecido.
    - VRBIO_ERROR_INVALID_TEMPLATE: O template é inválido.
    - VRBIO_ERROR_FAIL: Erro interno ao realizar o matching.
    - VRBIO_ERROR_FEATURE_NOT_LICENSED: Licença não permite este tipo de template.
    - VRBIO_ERROR_NOT_AVAILABLE_ON_FREE_EDITION: Tentando comparar templates de leitores biométricos diferentes, ou dos quais não seja possível determinar o leitor biométrico.
*/
int veridisbio_identifyEx(
        void* context,
  const char* tpt_buffer, 
         int  tpt_size, 
  const char* tpt_format);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Termina uma identificação 1-para-N, liberando os recursos alocados em \ref veridisbio_prepareIdentification.

  @param[in,out] context Ponteiro para a variavel com o contexto. A variavel irá conter NULL no retorno.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: \a context == \c NULL
*/
int veridisbio_terminateIdentification(void** context);


//@}####################################################################################################################################
///@name Consolidação de templates
//@{####################################################################################################################################


/**
  Inicia uma consolidação, capaz de unir várias imagens para obter um único template resultante, de maior qualidade.
  
  O uso de várias imagens permite aumentar a área útil (pois cada imagem é posicionada ligeiramente diferente) e uma melhor diferenciação entre pontos de interesse e ruído.
  
  Os melhores resultados tem sido obtidos com cerca de 5 imagens. 
  Também é ideal que se tenha o cuidado de não capturar imagens muito similares, o que pode ser obtido através 
  de um pequeno intervalo entre as capturas consecutivas.
  
  Para uma cada imagem que serão consolidada, é preciso usar \ref veridisbio_mergeImage.
  
  Quando todas as imagens tiverem sido adicionadas, \ref veridisbio_getMergeResult pode ser usado para obter o template resultante.
  
  Por fim, \ref veridisbio_terminateMerge deve ser utilizado para liberar os recursos alocados.
  
  @param[out] context         Variável que irá armazenar o <i>Contexto de Consolidação</i> criado.
  @param[in]  biometric_modality Tipo de biometria que será consolidado
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: context==NULL
    - VRBIO_ERROR_UNSUPPORTED_BIOMETRIC_MODALITY: A consolidação desta modalidade de biometria não é suportada pela bibioteca
    - VRBIO_ERROR_FEATURE_NOT_LICENSED: Licença não permite a consolidação deste tipo de biometria.
*/
int veridisbio_prepareMerge(
  void** context, 
   int   biometric_modality);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Adiciona uma imagem biométrica a uma consolidação.
  @param[in]  context  O contexto de consolidação ao qual a imagem será adicionada
  @param[in]  img      A imagem que será adicionada à consolidação.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: context==NULL ou img==NULL ou img->buffer==NULL.
    - VRBIO_ERROR_UNSUPPORTED_BIOMETRIC_MODALITY: A modalidade de biometria desta imagem é incompatível com a especificada em \ref veridisbio_prepareMerge.
    - VRBIO_ERROR_NOT_AVAILABLE_ON_FREE_EDITION: Tentando consolidar templates de leitores biométricos diferentes, ou dos quais não seja possível determinar o leitor biométrico.
*/
int veridisbio_mergeImage(
                        void* context, 
  const VrBio_BiometricImage* img);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Retorna um template resultante da consolidação das imagens adicionadas através de \ref veridisbio_mergeImage, no formato padrão (O formato padrão poderá depender do tipo de biometria, configurações, versão, licença, etc). 

  Não esqueça de liberar o template resultante com \ref veridisutil_templateFree.

  Na verdade, este método é apenas uma forma abreviada de \code veridisbio_getMergeResult(context, tpt_buffer, tpt_size, NULL, NULL) \endcode

  @see template_formats
  @param[in]  context  O contexto de consolidação utilizado
  @param[out] tpt_buffer Irá apontar para o template criado.
  @param[out] tpt_size Irá conter o tamanho do template criado. Pode ser NULL.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: context==NULL ou tpt_buffer==NULL
    - VRBIO_ERROR_FAIL: Nenhuma imagem foi adicionada, ou um erro interno aconteceu.
    - VRBIO_ERROR_UNSUPPORTED_FORMAT: Se a configuração com o formato de templates padrão for inválida.
    - VRBIO_ERROR_INVALID_TEMPLATE: O formato de templates escolhido não é compativel com o template criado. Provavelmente a biometria não é compatível com o tipo do template.
    - VRBIO_ERROR_FEATURE_NOT_LICENSED: Licença não permite a criação do tipo de template especificado.
*/
int veridisbio_getMergeResult(
  void*  context, 
  char** tpt_buffer, 
   int* tpt_size);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Retorna um template resultante da consolidação das imagens adicionadas através de \ref veridisbio_mergeImage, no formato especificado.

  Caso o formato seja nulo, o formato padrão será utilizado (O formato padrão poderá depender to tipo de biometria, configurações, versão, licença, etc)
  
  Não esqueça de liberar o template resultante com \ref veridisutil_templateFree.

  Adicionalmente, o nome "canônico" do formato utilizado será retornado em out_tpt_format. Este nome deverá ser liberado através de \ref veridisutil_stringFree.

  @see template_formats
  @param[in]  context  O contexto de consolidação utilizado
  @param[out] tpt_buffer Irá apontar para o template criado.
  @param[out] tpt_size Irá conter o tamanho do template criado. Pode ser NULL.
  @param[in]  in_tpt_format Formato no qual o template deverá ser serializado. NULL para usar o formato padrão.
  @param[out] out_tpt_format Nome "canônico" do formato no qual o template deverá ser serializado. NULL para descartá-lo.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: context==NULL ou tpt_buffer==NULL
    - VRBIO_ERROR_FAIL: Nenhuma imagem foi adicionada, ou um erro interno aconteceu.
    - VRBIO_ERROR_UNSUPPORTED_FORMAT: Se o formato especificado for inválido.
    - VRBIO_ERROR_INVALID_TEMPLATE: O formato de templates escolhido não é compativel com o template criado. Provavelmente a biometria não é compatível com o tipo do template.
    - VRBIO_ERROR_FEATURE_NOT_LICENSED: Licença não permite a criação do tipo de template especificado.
*/
int veridisbio_getMergeResultEx(
        void*  context, 
        char** tpt_buffer, 
         int*  tpt_size, 
  const char*  in_tpt_format, 
  const char** out_tpt_format);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Termina uma consolidação de templates, liberando os recursos.

  @param[in,out] context Ponteiro para a variavel com o contexto. A variavel irá conter NULL no retorno.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: \a context == \c NULL
*/
int veridisbio_terminateMerge(void** context);


//@}####################################################################################################################################
///@name Licenciamento
//@{####################################################################################################################################


/**
  Inicializa a biblioteca com a licença especificada.
  
  O texto poderá tanto ser a chave da sua licença (Licenciamento online), quanto o texto da licença (Obtido através de \ref veridisutil_getLicenseText):
  <ul>
    <li>
      Caso seja utilizada a chave da licença, a biblioteca irá se comunicar com o servidor de licenças online para obter os dados da sua licença.      
      
      Exemplos de chave da licença: 
      \verbatim W1GWEPCT54GFV7F \endverbatim
      \verbatim KYWNAHZJH9P6BFU \endverbatim
      \verbatim IGUVINN9KNJTAYA \endverbatim
      \verbatim UD17X3FV6JEIE4V \endverbatim
      
      A chave não diferencia entre maiusculas e minusculas.

      Utilizar a chave da licença é equivalente a utilizar o método \ref veridisutil_installLicenseKey.
    </li>
    
    <li>
      Caso o texto da licença seja utilizado, será usado o licenciamento local, o que evita acesso à internet.
      Isso torna o licenciamento mais rápido, e permite o uso de terminais offline.
        
      Caso a licença tenha expirado ou não tenha sido gerada na máquina atual, a biblioteca 
      irá renová-la online automaticamente. Por isso, se sua aplicação use o texto da licença para inicializar,
      é recomendado que o texto armazenado seja atualizado com o valor obtido por \ref veridisutil_getLicenseText 
      todas as vezes após a inicialização da licença.
      
      Exemplo de texto de licença:
      \verbatim
Veridis Biometric SDK - Permanent License (Single user)

License key: W1GWEPCT54GFV7F
Licensed to: The Very Big Corporation
Features: All devices and template formats are supported

Valid on:
   Hardware IDs: MAC Address 00:11:22:33:44:55
           from: 04/Mar/2011 17:09:03 UTC

This license file was created on 04/Mar/2011 17:09:03 UTC
At this moment, this license is being used by 1 users

  
======================== BEGIN LICENSE =========================
| wfkmjMbZSUDUY+wxjhG+1qpApH2SHyZFLD610IkQ3PL5WrR7V/tQT4GFnwI9 |
| AkG+qGV44Zq+hg3HtvB57b4gYRG+RGNuaM0AX/uid3oTWGguhRYr2QZVluXC |
| Y794NPvErZuULfqRa5TX/SWsFJE4Ce9Y/yJXwJtNftVclOq38sdoR1Vl9BYD |
| QuowOuVihGnQ/5CrsjHqjboPp+LjzCyll9GC0IPn5LB5MZIAbDSZnTeTd2Zg |
| EHYwtHV0OMozbbWW1Idb3nh2FxMxtvurvT3oQd7ATMMimd4zQdx+ZedthwsL |
| ir+2HqLfOja9hfnBbU9kX5kAeNn2p6TOEudD7KBJUKm4JNwrxzHDmFvc+XVQ |
| lJjy7MujlYbxdjhdqV34rRhLLJ4DdGP9ZrsNmxmOKVOD0PLp7k/mcHeJo6Rv |
| szZLWpP4wASA3e3SA6lA3G1sKfsy3eI4CCHnBj38E4NzTh1RudYFyPwDdsq6 |
| xwGcEd3ZCAKIVQktifa4KGju2T7gSPHUUFxLKZ4AfD6w9Ze+GT1c9hLcz4wf |
| Y9CvHCKkXqdjQGq3EDZk96AqxNeo3ZjgY5RtwOPzzKafI3PBpH8cNK+CTK55 |
| YaD48hvYMQ2acMZMTqg=                                         |
========================= END LICENSE ==========================\endverbatim

      Caso o texto da licença esteja expirado ou não seja válido para este hardware, o licenciamento online será utilizado como fallback automaticamente.

      \attention O método \ref veridisutil_installLicenseText é similar, mas não implementa o fallback automático para licenciamento online.
    </li>
  </ul>

  \attention  O servidor online de licenças opera sobre uma conexão HTTP comum, na porta 80, portanto a maioria das máquinas com acesso à internet deve ser capaz de utilizar o licensiamento online sem problemas. 
  
  \attention  Se a sua rede não permite a conexão direta, é possível fazer a validação da licença através de um servidor proxy HTTP comum, utilizando os métodos \ref veridisutil_installLicenseProxy e \ref veridisutil_installLicenseProxyAuth (Para proxys que exigem autenticação tipo Basic). 
  
  \attention  Se mesmo assim você tiver problemas, é possível implementar manualmente a comunicação com o servidor usando \ref veridisutil_makeLicenseRequest, 
  \ref veridisutil_installLicenseResponse e \ref veridisutil_freeLicenseRequest.

  @param[in] license Chave ou texto da licença.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: license==NULL
    - VRBIO_ERROR_INVALID_LICENSE: A chave da licença não é válida, ou o texto da licença está corrompido.
    - VRBIO_ERROR_LICENSE_EXPIRED: Licença está expirada, ou não é válida para este hardware.
    - VRBIO_ERROR_LICENSE_USERS_EXCEEDED: A licençá não pode ser usada neste computador, pois já atingiu o número máximo de usuários.
    - VRBIO_ERROR_HTTP_CONNECTION_FAIL: Não foi possível se conectar ao servidor de licençás.
    - VRBIO_ERROR_FAIL: Erro interno do servidor de licenças
*/
int veridisutil_installLicense(const char* license);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Inicializa a biblioteca com a licença especificada, acessando o servidor de licenças através de um Proxy HTTP.

  Exceto pelo uso de um Proxy HTTP, esta função é idêntica a \ref veridisutil_installLicense. Verifique a documentação deste método para mais detalhes.

  @param[in]  license Chave ou texto da licença.
  @param[in]  proxy_host Nome ou endereço do servidor Proxy HTTP. Especificar proxy_host==null implica em utilizar uma conexão direta (sem proxy).
  @param[in]  proxy_port Porta onde o serviço de Proxy HTTP está rodando.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: license==NULL
    - VRBIO_ERROR_INVALID_LICENSE: A chave da licença não é válida, ou o texto da licença está corrompido.
    - VRBIO_ERROR_LICENSE_EXPIRED: Licença está expirada, ou não é válida para este hardware.
    - VRBIO_ERROR_LICENSE_USERS_EXCEEDED: A licençá não pode ser usada neste computador, pois já atingiu o número máximo de usuários.
    - VRBIO_ERROR_HTTP_CONNECTION_FAIL: Não foi possível se conectar ao servidor de licençás.
    - VRBIO_ERROR_FAIL: Erro interno do servidor de licenças
*/
int veridisutil_installLicenseProxy(
  const char* license,
  const char* proxy_host,
         int  proxy_port);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Inicializa a biblioteca com a licença especificada, acessando o servidor de licenças através de um Proxy HTTP Autenticado.

  Exceto pelo uso de um Proxy HTTP autenticado, esta função é idêntica a \ref veridisutil_installLicense. Verifique a documentação deste método para mais detalhes.

  Atualmente, só é suportada autenticação do tipo "Basic" (Através da adição do cabeçalho "<tt>Proxy-authorization: Basic ******</tt>", conforme especificada pela <a href="http://www.ietf.org/rfc/rfc2617.txt">RFC 2617</a>).

  @param[in]  license Chave ou texto da licença.
  @param[in]  proxy_host Nome ou endereço do servidor Proxy HTTP. Especificar proxy_host==null implica em utilizar uma conexão direta (sem proxy).
  @param[in]  proxy_port Porta onde o serviço de Proxy HTTP está rodando.
  @param[in]  proxy_user Usuário do Proxy HTTP.
  @param[in]  proxy_password Senha do Proxy HTTP.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: license==NULL
    - VRBIO_ERROR_INVALID_LICENSE: A chave da licença não é válida, ou o texto da licença está corrompido.
    - VRBIO_ERROR_LICENSE_EXPIRED: Licença está expirada, ou não é válida para este hardware.
    - VRBIO_ERROR_LICENSE_USERS_EXCEEDED: A licençá não pode ser usada neste computador, pois já atingiu o número máximo de usuários.
    - VRBIO_ERROR_HTTP_CONNECTION_FAIL: Não foi possível se conectar ao servidor de licençás.
    - VRBIO_ERROR_FAIL: Erro interno do servidor de licenças
*/
int veridisutil_installLicenseProxyAuth(
  const char* license,
  const char* proxy_host,
         int  proxy_port,
  const char* proxy_user,
  const char* proxy_password);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Inicializa a biblioteca com a chave de licençá especificada. No processo, o servidor de licenças será consultado.

  \attention A forma preferencial de instalar licenças é pelo método \ref veridisutil_installLicense.

  O servidor online de licenças opera sobre uma conexão HTTP comum, na porta 80, portanto a maioria das máquinas com acesso à internet deve ser capaz de utilizar o licensiamento online sem problemas.
  Mas se a sua rede não permite a conexão direta, é possível fazer a validação da licença através de um servidor proxy HTTP comum, utilizando os métodos \ref veridisutil_installLicenseKeyProxy e \ref veridisutil_installLicenseKeyProxyAuth (Para proxys que exigem autenticação tipo Basic).

  Para detalhes sobre licenciamento, veja a documentação do método \ref veridisutil_installLicense.

  @param[in]  key Chave da licença.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: key==NULL
    - VRBIO_ERROR_INVALID_LICENSE: A chave fornecida não é válida
    - VRBIO_ERROR_LICENSE_EXPIRED: Licença está expirada, ou não é válida para este hardware.
    - VRBIO_ERROR_LICENSE_USERS_EXCEEDED: A licençá não pode ser usada neste computador, pois já atingiu o número máximo de usuários.
    - VRBIO_ERROR_HTTP_CONNECTION_FAIL: Não foi possível se conectar ao servidor de licençás.
    - VRBIO_ERROR_FAIL: Erro interno do servidor de licenças
*/
int veridisutil_installLicenseKey(const char* key);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Inicializa a biblioteca com a chave de licençá especificada. No processo, o servidor de licenças será consultado, através de um proxy HTTP.

  Exceto pelo uso de um Proxy HTTP, esta função é idêntica a \ref veridisutil_installLicenseKey.

  \attention A forma preferencial de instalar licenças é pelo método \ref veridisutil_installLicense. Para detalhes sobre licenciamento, veja a documentação deste método.

  @param[in]  key Chave da licença.
  @param[in]  proxy_host Nome ou endereço do servidor Proxy HTTP. Especificar proxy_host==null implica em utilizar uma conexão direta (sem proxy).
  @param[in]  proxy_port  Porta onde o serviço de Proxy HTTP está rodando. 
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: key==NULL
    - VRBIO_ERROR_INVALID_LICENSE: A chave fornecida não é válida
    - VRBIO_ERROR_LICENSE_EXPIRED: Licença está expirada, ou não é válida para este hardware.
    - VRBIO_ERROR_LICENSE_USERS_EXCEEDED: A licençá não pode ser usada neste computador, pois já atingiu o número máximo de usuários.
    - VRBIO_ERROR_HTTP_CONNECTION_FAIL: Não foi possível se conectar ao servidor de licençás.
    - VRBIO_ERROR_FAIL: Erro interno do servidor de licenças
*/


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


int veridisutil_installLicenseKeyProxy(const char* key, const char* proxy_host, int proxy_port);
/**
  Inicializa a biblioteca com a chave de licençá especificada. No processo, o servidor de licenças será consultado, através de um proxy HTTP  Autenticado.

  Exceto pelo uso de um Proxy HTTP autenticado, esta função é idêntica a \ref veridisutil_installLicenseKey.

  \attention Atualmente, só é suportada autenticação do tipo "Basic" (Através da adição do cabeçalho "<tt>Proxy-authorization: Basic ******</tt>", conforme especificada pela <a href="http://www.ietf.org/rfc/rfc2617.txt">RFC 2617</a>).
    
  \attention A forma preferencial de instalar licenças é pelo método \ref veridisutil_installLicense. Para detalhes sobre licenciamento, veja a documentação deste método.

  @param[in]  key Chave da licença.
  @param[in]  proxy_host Nome ou endereço do servidor Proxy HTTP. Especificar proxy_host==null implica em utilizar uma conexão direta (sem proxy).
  @param[in]  proxy_port Porta onde o serviço de Proxy HTTP está rodando.
  @param[in]  proxy_user Nome de usuário sendo autenticado. Especificar proxy_user==null implica em fazer uma requisição não-autenticada.
  @param[in]  proxy_pass Senha do usuário sendo autenticado.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: key==NULL
    - VRBIO_ERROR_INVALID_LICENSE: A chave fornecida não é válida
    - VRBIO_ERROR_LICENSE_EXPIRED: Licença está expirada, ou não é válida para este hardware.
    - VRBIO_ERROR_LICENSE_USERS_EXCEEDED: A licençá não pode ser usada neste computador, pois já atingiu o número máximo de usuários.
    - VRBIO_ERROR_HTTP_CONNECTION_FAIL: Não foi possível se conectar ao servidor de licençás.
    - VRBIO_ERROR_FAIL: Erro interno do servidor de licenças
*/
int veridisutil_installLicenseKeyProxyAuth(const char* key, const char* proxy_host, int proxy_port, const char* proxy_user, const char* proxy_pass);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Permite que usuário implemente o acesso ao servidor de licenças usando suas próprias bibliotecas de comunicação HTTP.
  
  \attention Quando o acesso à internet pode ser realizado diretamente, através de um servidor proxy HTTP comum, ou um servidor proxy HTTP com autenticação "Basic", os métodos \ref veridisutil_installLicense, \ref veridisutil_installLicenseProxy e \ref veridisutil_installLicenseProxyAuth são suficientes. 

  Ao fim deste método, \a request irá conter o URL e o corpo da requisição POST que deve ser enviada. Tanto a requisição quanto a resposta são codificadas como texto não formatado (Content-Type: text/plain;charset=utf-8).
 
  O usuário deverá obter a resposta e instalá-la com o método \ref veridisutil_installLicenseResponse.
 
  Por fim, a \a request deverá ser liberada com \ref veridisutil_freeLicenseRequest.
 
  @param[in]  key Chave da licença.
  @param[out] request Dados da requisição que deverá ser realizada.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: key==NULL ou request==NULL
    - VRBIO_ERROR_INVALID_LICENSE: A chave fornecida não é válida.
 */
int veridisutil_makeLicenseRequest(const char* key, const LicenseRequest** request);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Recebe a resposta de uma requisição de licença criada pelo método \ref veridisutil_makeLicenseRequest.

  O método deverá receber a requisição, criada pelo método \ref veridisutil_makeLicenseRequest, e a resposta, retornada pelo servidor.
 
  @param[in]  request Requisição sendo respondida.
  @param[out] response Resposta do servidor de licenças.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: request==NULL ou response==NULL
    - VRBIO_ERROR_INVALID_LICENSE: A resposta fornecida não é válida.
    - VRBIO_ERROR_LICENSE_EXPIRED: Licença está expirada, ou não é válida para este hardware.
    - VRBIO_ERROR_LICENSE_USERS_EXCEEDED: A licençá não pode ser usada neste computador, pois já atingiu o número máximo de usuários.
    - VRBIO_ERROR_FAIL: O servidor de licenças teve um erro durante o processamento da requisição.  
 */
int veridisutil_installLicenseResponse(const LicenseRequest* request, const char* response);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Libera uma \ref LicenseRequest alocada pela biblioteca. (Em especial, \ref veridisutil_makeLicenseRequest)

  @param[in,out] request Ponteiro para a variavel com a LicenseRequest que será liberada. A variavel irá conter NULL no retorno.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: value == NULL
*/
int veridisutil_freeLicenseRequest(const LicenseRequest** request);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Inicializa a biblioteca com a licença texto especificada, sem nunca tentar acessar o servidor de licenças.

  \attention A forma preferencial de instalar licenças é pelo método \ref veridisutil_installLicense. Para detalhes sobre licenciamento, veja a documentação deste método.

  @see veridisutil_installLicense
  @see veridisutil_installLicenseTextEx
  @param[in]  text Texto da licença.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: text==NULL
    - VRBIO_ERROR_INVALID_LICENSE: O texto da licença está corrompido.
    - VRBIO_ERROR_LICENSE_EXPIRED: Licença está expirada, ou não é válida para este hardware.
*/
int veridisutil_installLicenseText(const char* text);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Inicializa a biblioteca com a licença texto especificada, sem nunca tentar acessar o servidor de licenças.

  Se o texto da licença for válido, mesmo que a licença não puder ser instalada (retorno VRBIO_ERROR_LICENSE_EXPIRED),
  o parâmetro \a key retornará com a chave da licença. Esta string deverá ser liberada com \ref veridisutil_stringFree.

  \attention A forma preferencial de instalar licenças é pelo método \ref veridisutil_installLicense. Para detalhes sobre licenciamento, veja a documentação deste método.

  @param[in]  text Texto da licença.
  @param[out] key  Chave da licença. Pode ser NULL.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: text==NULL
    - VRBIO_ERROR_INVALID_LICENSE: O texto da licença está corrompido.
    - VRBIO_ERROR_LICENSE_EXPIRED: Licença está expirada, ou não é válida para este hardware.
*/
int veridisutil_installLicenseTextEx(const char* text, const char** key);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Retorna um buffer de texto com informações da licença atual que pode ser utilizado para licenciamento offline nas próximas execuções da aplicação. 
  
  Detalhes do licenciamento offline na documentação de \ref veridisutil_installLicense.
  
  Este método é apenas uma forma abreviada do seguinte código: 
  
  \code veridisutil_getLicenseTextEx(VRBIO_LICENSE_FULL_TEXT, licenseText)\endcode
  
  O texto retornado deverá ser liberado com \ref veridisutil_stringFree.

  @see veridisutil_getLicenseTextEx
  @param[out] licenseText Irá apontar para o texto da licença.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: licenseText==NULL
*/
int veridisutil_getLicenseText(const char** licenseText);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Consulta informações da licença instalada com \ref veridisutil_installLicense.
  
  As informações disponíveis atualmente são:
  <ul>
     <li>\ref VrBio_LicenseProperty::VRBIO_LICENSE_KEY : Chave da licença.
     
         Exemplo: 
         \verbatim W1GWEPCT54GFV7F \endverbatim
     </li>
     
     <li>\ref VrBio_LicenseProperty::VRBIO_LICENSE_USER : Dono da licença.
     
         Exemplo: 
         \verbatim The Very Big Corporation \endverbatim
     </li>
     
     <li>\ref VrBio_LicenseProperty::VRBIO_LICENSE_TEXT : Texto da licença, uma descrição com detalhes da licença, tais como validade, recursos habilitados no SDK, etc.
     
         Exemplo:
         \verbatim
Veridis Biometric SDK - Permanent License (Single user)

License key: W1GWEPCT54GFV7F
Licensed to: The Very Big Corporation
Features: All devices and template formats are supported

Valid on:
   Hardware IDs: MAC Address 00:11:22:33:44:55
           from: 04/Mar/2011 17:09:03 UTC

This license file was created on 04/Mar/2011 17:09:03 UTC
At this moment, this license is being used by 1 users\endverbatim

     </li>
     
     <li>\ref VrBio_LicenseProperty::VRBIO_LICENSE_FULL_TEXT : Texto da licença seguido de um bloco de dados binário, que pode ser utilizado para licenciar a biblioteca offline nas próximas vezes. 
     
     Detalhes do licenciamento offline na documentação de \ref veridisutil_installLicense.
     
         Exemplo:
        \verbatim
Veridis Biometric SDK - Permanent License (Single user)

License key: W1GWEPCT54GFV7F
Licensed to: The Very Big Corporation
Features: All devices and template formats are supported

Valid on:
   Hardware IDs: MAC Address 00:11:22:33:44:55
           from: 04/Mar/2011 17:09:03 UTC

This license file was created on 04/Mar/2011 17:09:03 UTC
At this moment, this license is being used by 1 users

  
======================== BEGIN LICENSE =========================
| wfkmjMbZSUDUY+wxjhG+1qpApH2SHyZFLD610IkQ3PL5WrR7V/tQT4GFnwI9 |
| AkG+qGV44Zq+hg3HtvB57b4gYRG+RGNuaM0AX/uid3oTWGguhRYr2QZVluXC |
| Y794NPvErZuULfqRa5TX/SWsFJE4Ce9Y/yJXwJtNftVclOq38sdoR1Vl9BYD |
| QuowOuVihGnQ/5CrsjHqjboPp+LjzCyll9GC0IPn5LB5MZIAbDSZnTeTd2Zg |
| EHYwtHV0OMozbbWW1Idb3nh2FxMxtvurvT3oQd7ATMMimd4zQdx+ZedthwsL |
| ir+2HqLfOja9hfnBbU9kX5kAeNn2p6TOEudD7KBJUKm4JNwrxzHDmFvc+XVQ |
| lJjy7MujlYbxdjhdqV34rRhLLJ4DdGP9ZrsNmxmOKVOD0PLp7k/mcHeJo6Rv |
| szZLWpP4wASA3e3SA6lA3G1sKfsy3eI4CCHnBj38E4NzTh1RudYFyPwDdsq6 |
| xwGcEd3ZCAKIVQktifa4KGju2T7gSPHUUFxLKZ4AfD6w9Ze+GT1c9hLcz4wf |
| Y9CvHCKkXqdjQGq3EDZk96AqxNeo3ZjgY5RtwOPzzKafI3PBpH8cNK+CTK55 |
| YaD48hvYMQ2acMZMTqg=                                         |
========================= END LICENSE ==========================\endverbatim

     </li>     
  </ul>
  Não esqueça de chamar \ref veridisutil_stringFree depois!

  @see veridisutil_installLicense
  @param[in]  code Qual informação da licença está sendo consultada. Lista dos valores aceitos em \ref VrBio_LicenseProperty.
  @param[out] value Irá apontar para o com a informação desejada.
  @return Código de erro.
    - VRBIO_SUCCESS : OK
    - VRBIO_ERROR_ARGUMENT : licenseText==NULL.
    - VRBIO_ERROR_UNSUPORTED_OPERATION : O valor de \a code não é suportado.
*/
int veridisutil_getLicenseTextEx(
         int   code, 
  const char** value);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Verifica se a biblioteca já foi licenciada (com \ref veridisutil_installLicense).
   
  \deprecated A partir da versão 3.1, este método sempre retorna \ref VRBIO_SUCCESS, pois o SDK vem com uma licença gratuita (FREE) pré-instalada.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
*/
int veridisutil_isLicensed();
  
  
//@}####################################################################################################################################
///@name Versão da biblioteca
//@{####################################################################################################################################


/**
  Retorna a versão deste SDK
  @param[out] major Numero principal da versão
  @param[out] minor Numero secundário da versão
  @param[out] revision Revisão
  @param[out] build Número de build
  @return Código de erro
    - VRBIO_SUCCESS: OK
*/
int veridisutil_getVersion(
  int* major, 
  int* minor, 
  int* revision, 
  int* build);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Retorna a data em que este SDK foi lançado.
  
  Todos os parâmetros são opcionais: Se você quer saber apenas um dos campos, basta usar NULL nos demais.
  
  Este método é apenas uma forma abreviada do seguinte código: \code veridisutil_getVersionDateTime(year, month, day, NULL, NULL, NULL)\endcode
  
  @param[out] year   Ano do lançamento
  @param[out] month  Mês do lançamento
  @param[out] day    Dia do lançamento
  @return Código de erro
    - VRBIO_SUCCESS: OK
*/
int veridisutil_getVersionDate(
  int* year, 
  int* month, 
  int* day);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Retorna a data e a hora em que este SDK foi compilado.
  
  A hora é informada no fuso-horário UTC / GMT.

  Todos os parâmetros são opcionais: Se você quer saber apenas um dos campos, basta usar NULL nos demais.

  @param[out] year    Ano do build
  @param[out] month   Mês do build
  @param[out] day     Dia do build
  @param[out] hour    Hora do horário de build (0..23)
  @param[out] minute  Minutos do horário de build (0..59)
  @param[out] second  Segundos do horário de build (0..59)
  @return Código de erro
    - VRBIO_SUCCESS: OK
*/
int veridisutil_getVersionDateTime(
  int* year, 
  int* month, 
  int* day, 
  int* hour, 
  int* minute, 
  int* second);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Retorna o nome desta biblioteca ("Veridis Biometric SDK")
  
  O nome deverá ser liberado com \ref veridisutil_stringFree.
  
  @param[out] productName O nome desta biblioteca.
  @return Código de erro
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: \a productName==NULL.
*/
int veridisutil_getProductName(const char** productName);
  
  
//@}####################################################################################################################################
///@name Liberação de recursos alocados.
//@{####################################################################################################################################


/**
  Libera um template criado por \ref veridisbio_extract ou \ref veridisbio_extractEx.
  
  @param[in,out] value Ponteiro para a variavel com o template que será liberado. A variavel irá conter NULL no retorno.
  @return Código de erro
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: \a value==NULL
*/
int veridisutil_templateFree(char** value);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Libera uma string alocada pela biblioteca. (Em especial, \ref veridiscap_getReaderString e \ref veridiscap_synchronousCapture)
  
  @param[in,out] value Ponteiro para a variavel com a string que será liberada. A variavel irá conter NULL no retorno.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: \a value == NULL
*/
int veridisutil_stringFree(const char** value);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
  Libera uma imagem alocada pela biblioteca. (Em especial, pela função \ref veridiscap_synchronousCapture)
  Não é necessário (E nem permitido) chamar essa função com imagens obtidas pelo callback.
  
  @param[in,out] image Ponteiro para a variavel com a imagem que será liberada. A variavel irá conter NULL no retorno.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: \a image == NULL
*/
int veridisutil_imageFree(VrBio_BiometricImage** image);

/**
  Libera um \ref VrBio_ReaderProperties alocado pela biblioteca.
  
  @param[in,out] properties Ponteiro para a variavel que será liberada. A variavel irá conter NULL no retorno.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: \a properties == NULL
*/
int veridisutil_ReaderPropertiesFree(VrBio_ReaderProperties** properties);

/**
  Libera um vetor de strings alocado pela biblioteca. (Em especial, pela função \ref veridiscap_listDevices)
  
  @param[in,out] array Ponteiro para a variavel com o vetor que será liberado. A variavel irá conter NULL no retorno.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: \a array == NULL
*/
int veridisutil_stringListFree(const char*** array);

/**
  Libera um vetor de \ref VrBio_ReaderProperties alocado pela biblioteca. (Em especial, pela função \ref veridiscap_listDevicesEx)
  
  @param[in,out] array Ponteiro para a variavel com o vetor que será liberado. A variavel irá conter NULL no retorno.
  @return Código de erro.
    - VRBIO_SUCCESS: OK
    - VRBIO_ERROR_ARGUMENT: \a array == NULL
*/
int veridisutil_ReaderPropertiesListFree(VrBio_ReaderProperties*** array);

//@}

#ifdef __cplusplus
} //extern "C"
#endif

#endif //ifndef VERIDIS_BIOMETRIC_H
