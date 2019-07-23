#include "sai_vs.h"
#include "sai_vs_internal.h"
#include "sai_vs_state.h"
#include "sai_vs_switch_BCM56850.h"
#include "sai_vs_switch_MLNX2700.h"

sai_status_t vs_clear_port_all_stats(
        _In_ sai_object_id_t port_id)
{
    MUTEX();

    SWSS_LOG_ENTER();

    return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t vs_create_port(
            _Out_ sai_object_id_t *port_id,
            _In_ sai_object_id_t switch_id,
            _In_ uint32_t attr_count,
            _In_ const sai_attribute_t *attr_list)
{
    MUTEX();
    SWSS_LOG_ENTER();

    /* create port */
    CHECK_STATUS(meta_sai_create_oid((sai_object_type_t)SAI_OBJECT_TYPE_PORT,
                port_id,switch_id,attr_count,attr_list,&vs_generic_create));

    if (g_vs_switch_type == SAI_VS_SWITCH_TYPE_BCM56850)
    {
        vs_create_port_BCM56850(*port_id, switch_id);
    }
    else if (g_vs_switch_type == SAI_VS_SWITCH_TYPE_MLNX2700)
    {
        vs_create_port_MLNX2700(*port_id, switch_id);
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t vs_set_port_attribute(
            _In_ sai_object_id_t port_id,
            _In_ const sai_attribute_t *attr)
{
    MUTEX();
    SWSS_LOG_ENTER();
    SWSS_LOG_ERROR("vs_set_port_attribute: SAMPLE : entered");
    // For samplepacket attr, 'tc sample' command is invoked
    if (attr->id == SAI_PORT_ATTR_INGRESS_SAMPLEPACKET_ENABLE){
        // Get the samplepacket object id
        sai_object_id_t samplepacket_oid = attr->value.oid;
        SWSS_LOG_ERROR("vs_set_port_attribute: SAMPLE : get the sample obj");

        // Get the if_name from the port_id
        std::string if_name;
        if(g_vs_switch_id == nullptr){
              SWSS_LOG_ERROR("null switch id");
              return SAI_STATUS_FAILURE;
        }

        auto it = g_switch_state_map.find(*g_vs_switch_id);
        if (it == g_switch_state_map.end())
        {
              SWSS_LOG_ERROR("failed to get switch state for switch id %s",
                 sai_serialize_object_id(*g_vs_switch_id).c_str());
              return SAI_STATUS_FAILURE;
        }
        SWSS_LOG_ERROR("vs_set_port_attribute: SAMPLE : got switch state");

        auto sw = it->second;
        if (sw == nullptr)
        {
              SWSS_LOG_ERROR("null switch id");
              return SAI_STATUS_FAILURE;
        }
        SWSS_LOG_ERROR("vs_set_port_attribute: SAMPLE : got switch id");

        if(sw->getPortIDToIfName(port_id, if_name) == false){
              SWSS_LOG_ERROR("Unable to get if_name mapped to the port_id, sampling session is not updated");
              return SAI_STATUS_FAILURE;
        }
        SWSS_LOG_ERROR("vs_set_port_attribute: SAMPLE : if_name: %s",if_name);


        if(samplepacket_oid == SAI_NULL_OBJECT_ID)
        {
              //Delete the sampling session
              SWSS_LOG_ERROR("vs_set_port_attribute: SAMPLE : Executing tc sample delete");
              std::string cmd = "tc qdisc delete dev " + if_name + " handle ffff: ingress";
              if(system(cmd.c_str()) == -1){
                  SWSS_LOG_ERROR("Unable to delete the sampling session");
                  return SAI_STATUS_FAILURE;
              }
              SWSS_LOG_ERROR("vs_set_port_attribute: SAMPLE : Finished tc sample delete");
        } else {
              // Get the sample rate
              SWSS_LOG_ERROR("vs_set_port_attribute: SAMPLE : Executing tc sample update");
              sai_attribute_t samplepacket_attr;
              samplepacket_attr.id = SAI_SAMPLEPACKET_ATTR_SAMPLE_RATE;
              if(SAI_STATUS_SUCCESS == \
                 vs_generic_get(SAI_OBJECT_TYPE_SAMPLEPACKET, samplepacket_oid, 1, &samplepacket_attr))
              {
                  SWSS_LOG_ERROR("vs_set_port_attribute: SAMPLE : found sampling rate inside sample obj");
                  int rate = samplepacket_attr.value.u32;

                  // Default sample group ID
                  std::string group("1");

                  //Delete the sampling session
                  std::string cmd = "tc qdisc delete dev " + if_name + " handle ffff: ingress";
                  if(system(cmd.c_str()) == -1){
                      SWSS_LOG_ERROR("Unable to delete/update the sampling session");
                      return SAI_STATUS_FAILURE;
                  }
                  SWSS_LOG_ERROR("vs_set_port_attribute: SAMPLE : Executed tc sample delete");

                  //Create a new sampling session
                  cmd = "tc qdisc add dev " + if_name + " handle ffff: ingress";
                  if(system(cmd.c_str()) == -1){
                      SWSS_LOG_ERROR("Unable to add/update the sampling session");
                      return SAI_STATUS_FAILURE;
                  }
                  SWSS_LOG_ERROR("vs_set_port_attribute: SAMPLE : Executed tc qdisc add");

                  cmd = "tc filter add dev " + if_name + \
                        " parent ffff: matchall action sample rate " + std::to_string(rate) + \
                        " group " + group;
                  if(system(cmd.c_str()) == -1){
                      SWSS_LOG_ERROR("Unable to update the sampling session");
                      return SAI_STATUS_FAILURE;
                  }
                  SWSS_LOG_ERROR("vs_set_port_attribute: SAMPLE : Executed tc filter add");
              } else {
                  SWSS_LOG_ERROR("Unable to get SAI_SAMPLEPACKET_ATTR_SAMPLE_RATE, sampling session is not updated");
                  return SAI_STATUS_FAILURE;
              }
              SWSS_LOG_ERROR("vs_set_port_attribute: SAMPLE : Finished tc sample update");
        }
    }
    SWSS_LOG_ERROR("vs_set_port_attribute: SAMPLE: invoking vs_generic_set");

    return meta_sai_set_oid((sai_object_type_t)SAI_OBJECT_TYPE_PORT, port_id, attr, &vs_generic_set);
}

VS_REMOVE(PORT,port);
VS_GET(PORT,port);
VS_GENERIC_QUAD(PORT_POOL,port_pool);
VS_GENERIC_STATS(PORT,port);
VS_GENERIC_STATS(PORT_POOL,port_pool);

const sai_port_api_t vs_port_api = {

    VS_GENERIC_QUAD_API(port)
    VS_GENERIC_STATS_API(port)

    vs_clear_port_all_stats,

    VS_GENERIC_QUAD_API(port_pool)
    VS_GENERIC_STATS_API(port_pool)
};
