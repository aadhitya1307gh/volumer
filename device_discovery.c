#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pipewire/pipewire.h>

#define MAX_SINKS 32

//Struct to hold sink details for Developer B's engine
struct audio_sink {
	uint32_t id;
	char name[256];
	char description[256];
};

// Our local registry list of active sinks
struct audio_sink active_sinks[MAX_SINKS];
int sink_count = 0;

// Helper function to add a sink to the local array
static void add_sink(uint32_t id, const char *name, const char *desc) {
	if (sink_count < MAX_SINKS) {
		active_sinks[sink_count].id = id;
		snprintf(active_sinks[sink_count].name, sizeof(active_sinks[sink_count].name), "%s", name);
		snprintf(active_sinks[sink_count].description, sizeof(active_sinks[sink_count].description), "%s", desc);
		sink_count++;
		printf("[Registry] Added Sink ID %u (%s). Total active sinks: %d\n", id, desc, sink_count);
    }
}

// Helper function to remove a sink from the local array
static void remove_sink(uint32_t id) {
	for (int i = 0; i < sink_count; i++) {
		if (active_sinks[i].id == id) {
			printf("[Registry] Removing Sink ID %u (%s).\n", id, active_sinks[i].description);
			// Shift remaining elements down
			for (int j = i; j < sink_count - 1; j++) {
				active_sinks[j] = active_sinks[j + 1];
			}
			sink_count--;
			break;
		}
	}
}
//This callback runs everytime PipeWire sees an object on the system
static void registry_global(void *data, uint32_t id, uint32_t permissions, 
                            const char *type, uint32_t version, 
                            const struct spa_dict *props) {
	//We only need the Nodes
	 if (strcmp(type, PW_TYPE_INTERFACE_Node) == 0 && props != NULL) {
		 const char *media_class = spa_dict_lookup(props, "media.class");
        	// Looking for the audio sink 
		if (media_class && strcmp(media_class, "Audio/Sink") == 0) {
			const char *node_name = spa_dict_lookup(props, "node.name");
			const char *node_desc = spa_dict_lookup(props, "node.description");
			add_sink(id, node_name ? node_name : "unknown", node_desc ? node_desc : "unknown");
		}
	}
}

static void registry_global_remove(void *data, uint32_t id) {
	remove_sink(id);
}

static const struct pw_registry_events registry_events = {
	.version = PW_VERSION_REGISTRY,
	.global = registry_global,
	.global_remove = registry_global_remove,
};

int main(int argc, char *argv[]) {
	//Initializes PipeWire library
	pw_init(&argc, &argv);
	//Creates an asynchronous even loop. Engin to keep program running
	struct pw_main_loop *loop = pw_main_loop_new(NULL);
	//Establishes the connection pipeline to the local pipewire audio server running in your os
	struct pw_context *context = pw_context_new(pw_main_loop_get_loop(loop), NULL, 0);
	struct pw_core *core = pw_context_connect(context, NULL, 0);
	// Asks pw for the central registry, the place where all active objects on the system are noted
	struct pw_registry *registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);
    
	struct spa_hook registry_listener;
	pw_registry_add_listener(registry, &registry_listener, &registry_events, NULL);

	printf("Volumer audio discovery engine running...\n");
	//Loop starts and program blocks here
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
