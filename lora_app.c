#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <input/input.h>

// Configuration LoRa
#define LORA_FREQUENCY    868E6  // Fréquence européenne ISM
#define LORA_BANDWIDTH    125E3
#define LORA_SPREADING    7
#define LORA_CODING_RATE  5

typedef struct {
    uint8_t rx_buffer[64];
    uint8_t rx_len;
    bool message_received;
} LoRaApp;

// Initialisation du module LoRa
static bool init_lora(void) {
    // Configuration des pins GPIO
    furi_hal_gpio_init_simple(14, GpioModeInput);  // RX
    furi_hal_gpio_init_simple(13, GpioModeOutputPushPull);  // TX
    
    // Configuration de base LoRa
    if (!furi_hal_spi_acquire(&furi_hal_spi_bus_handle_external)) {
        return false;
    }
    
    // Configuration des paramètres LoRa
    lora_set_frequency(LORA_FREQUENCY);
    lora_set_bandwidth(LORA_BANDWIDTH);
    lora_set_spreading_factor(LORA_SPREADING);
    lora_set_coding_rate(LORA_CODING_RATE);
    
    return true;
}

// Fonction d'envoi de message
static void send_message(const char* message) {
    if (strlen(message) > 0) {
        lora_send_packet((uint8_t*)message, strlen(message));
    }
}

// Fonction de réception
static void receive_message(LoRaApp* app) {
    if (lora_available()) {
        app->rx_len = lora_receive_packet(app->rx_buffer, sizeof(app->rx_buffer));
        app->message_received = true;
    }
}

// Interface utilisateur
static void draw_callback(Canvas* canvas, void* ctx) {
    LoRaApp* app = ctx;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    
    canvas_draw_str(canvas, 2, 10, "LoRa Test App");
    canvas_draw_str(canvas, 2, 30, "Freq: 868MHz (ISM)");
    
    if (app->message_received) {
        canvas_draw_str(canvas, 2, 50, "Message recu:");
        canvas_draw_str(canvas, 2, 60, (char*)app->rx_buffer);
    }
}

// Gestionnaire d'entrées
static void input_callback(InputEvent* input_event, void* ctx) {
    LoRaApp* app = ctx;
    
    if (input_event->type == InputTypeShort) {
        if (input_event->key == InputKeyOk) {
            // Envoi d'un message test
            send_message("Test Message");
        }
    }
}

// Point d'entrée principal
int32_t lora_app(void* p) {
    UNUSED(p);
    LoRaApp* app = malloc(sizeof(LoRaApp));
    
    // Initialisation
    if (!init_lora()) {
        free(app);
        return -1;
    }
    
    // Configuration de l'interface
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, draw_callback, app);
    view_port_input_callback_set(view_port, input_callback, app);
    
    // Ajout à la GUI
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);
    
    // Boucle principale
    while(1) {
        receive_message(app);
        furi_delay_ms(100);
    }
    
    // Nettoyage
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);
    free(app);
    
    return 0;
}