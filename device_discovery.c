#include <stdio.h>
#include <string.h>
#include <pipewire/pipewire.h>

// This callback runs every time PipeWire sees an object on the system
static void registry_global(void *data, uint32_t id, uint32_t permissions, 
                            const char *type, uint32_t version, 
                            const struct spa_dict *props) {
    
    // We only care about Nodes (audio devices, streams, etc.)
    if (strcmp(type, PW_TYPE_INTERFACE_Node) == 0 && props != NULL) {
        
        // Look for the media.class property to find audio sinks (outputs)
        const char *media_class = spa_dict_lookup(props, "media.class");
        
        if (media_class && strcmp(media_class, "Audio/Sink") == 0) {
            const char *node_name = spa_dict_lookup(props, "node.name");
            const char *node_desc = spa_dict_lookup(props, "node.description");
            
            printf("[Found Audio Sink] ID: %u | Name: %s | Description: %s\n", 
                   id, node_name ? node_name : "unknown", node_desc ? node_desc : "unknown");
        }
    }
}

//Runs when any global object is removed from the system
static void registry_global_remove(void *data, uint32_t id){
	printf("[Disconnected] Object ID: %u has been removed from the system.\n", id);
};

static const struct pw_registry_events registry_events = {
    .version = PW_VERSION_REGISTRY,
    .global = registry_global,
    .global_remove = registry_global_remove, 
};

int main(int argc, char *argv[]) {
    pw_init(&argc, &argv);

    struct pw_main_loop *loop = pw_main_loop_new(NULL);
    struct pw_context *context = pw_context_new(pw_main_loop_get_loop(loop), NULL, 0);
    struct pw_core *core = pw_context_connect(context, NULL, 0);
    
    struct pw_registry *registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);
    
    struct spa_hook registry_listener;
    pw_registry_add_listener(registry, &registry_listener, &registry_events, NULL);

    printf("Scanning PipeWire registry for audio output devices...\n");

    pw_main_loop_run(loop);

    // Cleanup
    spa_hook_remove(&registry_listener);
    pw_proxy_destroy((struct pw_proxy *)registry);
    pw_core_disconnect(core);
    pw_context_destroy(context);
    pw_main_loop_destroy(loop);
    pw_deinit();

    return 0;
}
