"""
Unreal Engine Advanced MCP Server

A streamlined MCP server focused on advanced composition tools for Unreal Engine.
Contains only the advanced tools from the expanded MCP tool system to keep tool count manageable.
"""

import logging
import socket
import json
import math
import struct
import time
import threading
import base64
import os
from contextlib import asynccontextmanager
from typing import AsyncIterator, Dict, Any, Optional, List
from mcp.server.fastmcp import FastMCP

from helpers.infrastructure_creation import (
    _create_street_grid, _create_street_lights, _create_town_vehicles, _create_town_decorations,
    _create_traffic_lights, _create_street_signage, _create_sidewalks_crosswalks, _create_urban_furniture,
    _create_street_utilities, _create_central_plaza
)
from helpers.building_creation import _create_town_building
from helpers.castle_creation import (
    get_castle_size_params, calculate_scaled_dimensions, build_outer_bailey_walls, 
    build_inner_bailey_walls, build_gate_complex, build_corner_towers, 
    build_inner_corner_towers, build_intermediate_towers, build_central_keep, 
    build_courtyard_complex, build_bailey_annexes, build_siege_weapons, 
    build_village_settlement, build_drawbridge_and_moat, add_decorative_flags
)
from helpers.house_construction import build_house

from helpers.mansion_creation import (
    get_mansion_size_params, calculate_mansion_layout, build_mansion_main_structure,
    build_mansion_exterior, add_mansion_interior
)
from helpers.actor_utilities import spawn_blueprint_actor, get_blueprint_material_info
from helpers.actor_name_manager import (
    safe_spawn_actor, safe_delete_actor
)
from helpers.bridge_aqueduct_creation import (
    build_suspension_bridge_structure, build_aqueduct_structure
)

# ============================================================================
# Blueprint Node Graph Tools
# ============================================================================
from helpers.blueprint_graph import node_manager
from helpers.blueprint_graph import variable_manager
from helpers.blueprint_graph import connector_manager
from helpers.blueprint_graph import event_manager
from helpers.blueprint_graph import node_deleter
from helpers.blueprint_graph import node_properties
from helpers.blueprint_graph import function_manager
from helpers.blueprint_graph import function_io


# Configure logging with more detailed format
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s - %(name)s - %(levelname)s - [%(filename)s:%(lineno)d] - %(message)s',
    handlers=[
        logging.FileHandler(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'it_is_unreal.log')),
    ]
)
logger = logging.getLogger("ItIsUnreal")

# Configuration
UNREAL_HOST = os.environ.get("UNREAL_HOST", "127.0.0.1")
UNREAL_PORT = int(os.environ.get("UNREAL_PORT", "55557"))

class UnrealConnection:
    """
    Robust connection to Unreal Engine with automatic retry and reconnection.
    
    Features:
    - Exponential backoff retry for connection attempts
    - Automatic reconnection on failure
    - Configurable timeouts per command type
    - Thread-safe operations
    - Detailed logging for debugging
    """
    
    # Configuration constants
    MAX_RETRIES = 3
    BASE_RETRY_DELAY = 0.5  # seconds
    MAX_RETRY_DELAY = 5.0   # seconds
    CONNECT_TIMEOUT = 10    # seconds
    DEFAULT_RECV_TIMEOUT = 30  # seconds
    LARGE_OP_RECV_TIMEOUT = 300  # seconds for large operations
    BUFFER_SIZE = 8192
    
    # Commands that need longer timeouts
    LARGE_OPERATION_COMMANDS = {
        "get_available_materials",
        "create_town",
        "create_castle_fortress",
        "construct_mansion",
        "create_suspension_bridge",
        "create_aqueduct",
        "create_maze",
        "import_texture",
        "import_mesh",
        "import_skeletal_mesh",
        "import_animation",
        "import_sound",
        "create_pbr_material",
        "create_landscape_material",
        "scatter_meshes_on_landscape",
        "scatter_foliage",
        "create_widget_blueprint",
        "create_behavior_tree",
        "take_screenshot",
        "create_niagara_system",
        "create_atmospheric_fx",
    }

    # Commands that need a post-execution cooldown to let the engine
    # finish async work (texture compression, shader recompile, GC, streaming).
    # Without this delay, rapid successive calls cause memory pressure
    # that can crash the editor or corrupt landscape streaming proxies.
    HEAVY_COMMANDS_COOLDOWN = {
        "import_texture": 2.0,       # Texture decompression + compression + GPU upload
        "import_mesh": 3.0,          # FBX parsing + Nanite building + collision
        "import_skeletal_mesh": 5.0, # FBX parsing + skeleton + skin weights + physics asset
        "import_animation": 3.0,    # FBX parsing + animation curve processing
        "import_sound": 2.0,         # Sound decompression + asset creation
        "create_pbr_material": 1.0,  # Material compilation + shader compile
        "create_landscape_material": 2.0,  # Full material graph + shader compile
        "scatter_meshes_on_landscape": 2.0,  # Multiple spawns + line traces
        "scatter_foliage": 2.0,              # HISM scatter + Poisson disk + line traces
        "create_anim_montage": 1.0,      # Montage asset creation + save
        "create_widget_blueprint": 1.5,   # Widget BP creation + compile + save
        "create_behavior_tree": 1.0,      # BT asset creation + save
        "create_blackboard": 1.0,         # BB asset creation + save
        "take_screenshot": 1.0,          # SceneCapture2D render + ReadPixels + PNG encode
        "create_niagara_system": 2.0,    # Niagara system creation + compile + save
        "create_atmospheric_fx": 3.0,   # Niagara system with module stack + compile
    }
    
    def __init__(self):
        """Initialize the connection."""
        self.socket = None
        self.connected = False
        self._lock = threading.RLock()  # RLock allows reentrant acquisition for retry logic
        self._last_error = None
    
    def _create_socket(self) -> socket.socket:
        """Create and configure a new socket."""
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(self.CONNECT_TIMEOUT)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 131072)  # 128KB
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 131072)  # 128KB
        
        # Set linger to ensure clean socket closure (l_onoff=1, l_linger=0)
        # struct linger is two 16-bit integers: l_onoff and l_linger
        try:
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack('hh', 1, 0))
        except OSError:
            pass
        
        return sock
    
    def connect(self) -> bool:
        """
        Connect to Unreal Engine with retry logic.
        
        Uses exponential backoff for retries. Sleep occurs outside the lock
        to avoid blocking other threads during retry delays.
            
        Returns:
            True if connected successfully, False otherwise
        """
        for attempt in range(self.MAX_RETRIES + 1):
            # Hold lock only during connection attempt, not during sleep
            with self._lock:
                # Clean up any existing connection
                self._close_socket_unsafe()
                
                try:
                    logger.info(f"Connecting to Unreal at {UNREAL_HOST}:{UNREAL_PORT} (attempt {attempt + 1}/{self.MAX_RETRIES + 1})...")
                    
                    self.socket = self._create_socket()
                    self.socket.connect((UNREAL_HOST, UNREAL_PORT))
                    self.connected = True
                    self._last_error = None
                    
                    logger.info("Successfully connected to Unreal Engine")
                    return True
                    
                except socket.timeout as e:
                    self._last_error = f"Connection timeout: {e}"
                    logger.warning(f"Connection timeout (attempt {attempt + 1})")
                except ConnectionRefusedError as e:
                    self._last_error = f"Connection refused: {e}"
                    logger.warning(f"Connection refused - is Unreal Engine running? (attempt {attempt + 1})")
                except OSError as e:
                    self._last_error = f"OS error: {e}"
                    logger.warning(f"OS error during connection: {e} (attempt {attempt + 1})")
                except Exception as e:
                    self._last_error = f"Unexpected error: {e}"
                    logger.error(f"Unexpected connection error: {e} (attempt {attempt + 1})")
                
                self._close_socket_unsafe()
                self.connected = False
            
            # Sleep OUTSIDE the lock to allow other threads to proceed
            if attempt < self.MAX_RETRIES:
                delay = min(self.BASE_RETRY_DELAY * (2 ** attempt), self.MAX_RETRY_DELAY)
                logger.info(f"Retrying connection in {delay:.1f}s...")
                time.sleep(delay)
        
        logger.error(f"Failed to connect after {self.MAX_RETRIES + 1} attempts. Last error: {self._last_error}")
        return False
    
    def _close_socket_unsafe(self):
        """Close socket without lock (internal use only)."""
        if self.socket:
            try:
                self.socket.shutdown(socket.SHUT_RDWR)
            except:
                pass
            try:
                self.socket.close()
            except:
                pass
            self.socket = None
        self.connected = False
    
    def disconnect(self):
        """Safely disconnect from Unreal Engine."""
        with self._lock:
            self._close_socket_unsafe()
            logger.debug("Disconnected from Unreal Engine")

    def _get_timeout_for_command(self, command_type: str) -> int:
        """Get appropriate timeout for command type."""
        if any(large_cmd in command_type for large_cmd in self.LARGE_OPERATION_COMMANDS):
            return self.LARGE_OP_RECV_TIMEOUT
        return self.DEFAULT_RECV_TIMEOUT

    def _receive_response(self, command_type: str) -> bytes:
        """
        Receive complete JSON response from Unreal.
        
        Args:
            command_type: Type of command (used for timeout selection)
            
        Returns:
            Raw response bytes
            
        Raises:
            Exception: On timeout or connection error
        """
        timeout = self._get_timeout_for_command(command_type)
        self.socket.settimeout(timeout)
        
        chunks = []
        total_bytes = 0
        start_time = time.time()
        
        try:
            while True:
                # Check for overall timeout
                elapsed = time.time() - start_time
                if elapsed > timeout:
                    raise socket.timeout(f"Overall timeout after {elapsed:.1f}s")
                
                try:
                    chunk = self.socket.recv(self.BUFFER_SIZE)
                except socket.timeout:
                    # Check if we have a complete response
                    if chunks:
                        data = b''.join(chunks)
                        try:
                            json.loads(data.decode('utf-8'))
                            logger.info(f"Got complete response after recv timeout ({total_bytes} bytes)")
                            return data
                        except json.JSONDecodeError:
                            pass
                    raise
                
                if not chunk:
                    # Connection closed by remote
                    if not chunks:
                        raise ConnectionError("Connection closed before receiving any data")
                    break
                
                chunks.append(chunk)
                total_bytes += len(chunk)
                
                # Try to parse accumulated data as JSON
                data = b''.join(chunks)
                try:
                    decoded = data.decode('utf-8')
                    json.loads(decoded)
                    # Successfully parsed - we have complete response
                    logger.info(f"Received complete response ({total_bytes} bytes) for {command_type}")
                    return data
                except json.JSONDecodeError:
                    # Incomplete JSON, continue reading
                    continue
                except UnicodeDecodeError:
                    # Incomplete UTF-8, continue reading
                    continue
                    
        except socket.timeout:
            elapsed = time.time() - start_time
            if chunks:
                data = b''.join(chunks)
                try:
                    json.loads(data.decode('utf-8'))
                    logger.warning(f"Using response received before timeout ({total_bytes} bytes)")
                    return data
                except:
                    pass
            raise TimeoutError(f"Timeout after {elapsed:.1f}s waiting for response to {command_type} (received {total_bytes} bytes)")
        
        # If we get here, connection was closed
        if chunks:
            data = b''.join(chunks)
            try:
                json.loads(data.decode('utf-8'))
                return data
            except:
                raise ConnectionError(f"Connection closed with incomplete data ({total_bytes} bytes)")
        
        raise ConnectionError("Connection closed without response")

    def send_command(self, command: str, params: Dict[str, Any] = None) -> Optional[Dict[str, Any]]:
        """
        Send a command to Unreal Engine with automatic retry.

        Args:
            command: Command type string
            params: Command parameters dictionary

        Returns:
            Response dictionary or error dictionary
        """
        last_error = None

        for attempt in range(self.MAX_RETRIES + 1):
            try:
                result = self._send_command_once(command, params, attempt)

                # Apply post-command cooldown for heavy operations.
                # This gives the engine time to finish async work (texture compression,
                # shader recompile, GC, streaming updates) before the next command.
                cooldown = self.HEAVY_COMMANDS_COOLDOWN.get(command, 0)
                if cooldown > 0 and result.get("status") != "error":
                    logger.info(f"Heavy command '{command}' completed - cooling down {cooldown}s for engine stability")
                    time.sleep(cooldown)

                return result
            except (ConnectionError, TimeoutError, socket.error, OSError) as e:
                last_error = str(e)
                logger.warning(f"Command failed (attempt {attempt + 1}/{self.MAX_RETRIES + 1}): {e}")
                
                # Clean up and prepare for retry
                self.disconnect()
                
                if attempt < self.MAX_RETRIES:
                    delay = min(self.BASE_RETRY_DELAY * (2 ** attempt), self.MAX_RETRY_DELAY)
                    logger.info(f"Retrying command in {delay:.1f}s...")
                    time.sleep(delay)
            except Exception as e:
                # Unexpected error - don't retry
                logger.error(f"Unexpected error sending command: {e}")
                self.disconnect()
                return {"status": "error", "error": str(e)}
        
        return {"status": "error", "error": f"Command failed after {self.MAX_RETRIES + 1} attempts: {last_error}"}

    def _send_command_once(self, command: str, params: Dict[str, Any], attempt: int) -> Dict[str, Any]:
        """
        Send command once (internal method).
        
        Args:
            command: Command type
            params: Command parameters
            attempt: Current attempt number
            
        Returns:
            Response dictionary
            
        Raises:
            Various exceptions on failure
        """
        # Hold lock for entire send-receive cycle to prevent race conditions
        # where another thread could close/reconnect the socket mid-operation.
        # RLock allows nested acquisition from connect()/disconnect() calls.
        with self._lock:
            # Connect (or reconnect)
            if not self.connect():
                raise ConnectionError(f"Failed to connect to Unreal Engine: {self._last_error}")
            
            try:
                # Build and send command
                command_obj = {
                    "type": command,
                    "params": params or {}
                }
                command_json = json.dumps(command_obj)
                
                logger.info(f"Sending command (attempt {attempt + 1}): {command}")
                logger.debug(f"Command payload: {command_json[:500]}...")
                
                # Send with timeout
                self.socket.settimeout(10)  # 10 second send timeout
                self.socket.sendall(command_json.encode('utf-8'))
                
                # Receive response
                response_data = self._receive_response(command)
                
                # Parse response
                try:
                    response = json.loads(response_data.decode('utf-8'))
                except json.JSONDecodeError as e:
                    logger.error(f"JSON decode error: {e}")
                    logger.debug(f"Raw response: {response_data[:500]}")
                    raise ValueError(f"Invalid JSON response: {e}")
                
                logger.info(f"Command {command} completed successfully")
                
                # Normalize error responses
                if response.get("status") == "error":
                    error_msg = response.get("error") or response.get("message", "Unknown error")
                    logger.warning(f"Unreal returned error: {error_msg}")
                elif response.get("success") is False:
                    error_msg = response.get("error") or response.get("message", "Unknown error")
                    response = {"status": "error", "error": error_msg}
                    logger.warning(f"Unreal returned failure: {error_msg}")
                
                return response
                
            finally:
                # Always clean up connection after command
                self._close_socket_unsafe()

# Global connection instance (singleton pattern)
_unreal_connection: Optional[UnrealConnection] = None
_connection_lock = threading.Lock()

def get_unreal_connection() -> UnrealConnection:
    """
    Get the global Unreal connection instance.
    
    Uses lazy initialization - connection is created on first access.
    The connection handles its own retry logic, so we don't need to
    pre-connect here.
    
    Returns:
        UnrealConnection instance (always returns an instance, never None)
    """
    global _unreal_connection
    
    with _connection_lock:
        if _unreal_connection is None:
            logger.info("Creating new UnrealConnection instance")
            _unreal_connection = UnrealConnection()
        return _unreal_connection


def reset_unreal_connection():
    """Reset the global connection (useful for error recovery)."""
    global _unreal_connection
    
    with _connection_lock:
        if _unreal_connection:
            _unreal_connection.disconnect()
            _unreal_connection = None
        logger.info("Unreal connection reset")

@asynccontextmanager
async def server_lifespan(server: FastMCP) -> AsyncIterator[Dict[str, Any]]:
    """Handle server startup and shutdown."""
    logger.info("UnrealMCP Advanced server starting up")
    logger.info("Connection will be established lazily on first tool call")

    try:
        yield {}
    finally:
        reset_unreal_connection()
        logger.info("Unreal MCP Advanced server shut down")

# Initialize server
mcp = FastMCP(
    "it-is-unreal",
    lifespan=server_lifespan
)

# Essential Actor Management Tools
@mcp.tool()
def get_actors_in_level(random_string: str = "") -> Dict[str, Any]:
    """Get a list of all actors in the current level."""
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}
    
    try:
        response = unreal.send_command("get_actors_in_level", {})
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"get_actors_in_level error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def find_actors_by_name(pattern: str) -> Dict[str, Any]:
    """Find actors by name pattern."""
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}
    
    try:
        response = unreal.send_command("find_actors_by_name", {"pattern": pattern})
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"find_actors_by_name error: {e}")
        return {"success": False, "message": str(e)}



@mcp.tool()
def delete_actor(name: str) -> Dict[str, Any]:
    """Delete an actor by name."""
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}
    
    try:
        # Use the safe delete function to update tracking
        response = safe_delete_actor(unreal, name)
        return response
    except Exception as e:
        logger.error(f"delete_actor error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def delete_actors_by_pattern(pattern: str) -> Dict[str, Any]:
    """
    Delete all actors whose name contains the given pattern.

    Bulk delete operation - deletes every actor in the level whose name
    includes the pattern string. Useful for cleaning up groups of actors
    (e.g., pattern="Rock_" deletes all rock actors).

    Parameters:
    - pattern: Substring to match against actor names (case-sensitive)

    Returns:
        Dictionary with deleted_count, deleted_actors list, and pattern used.
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {"pattern": pattern}
        response = unreal.send_command("delete_actors_by_pattern", params)
        return response.get("result", response)
    except Exception as e:
        logger.error(f"delete_actors_by_pattern error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def set_actor_transform(
    name: str,
    location: List[float] = None,
    rotation: List[float] = None,
    scale: List[float] = None
) -> Dict[str, Any]:
    """Set the transform of an actor."""
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}
    
    try:
        params = {"name": name}
        if location is not None:
            params["location"] = location
        if rotation is not None:
            params["rotation"] = rotation
        if scale is not None:
            params["scale"] = scale
            
        response = unreal.send_command("set_actor_transform", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"set_actor_transform error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def set_actor_property(
    actor_name: str,
    property_name: str,
    property_value: Any,
    component_name: str = ""
) -> Dict[str, Any]:
    """
    Set a property on an actor or its component using Unreal's reflection system.

    This tool allows you to modify properties on actors and their components,
    including lights, fog, atmosphere, post-process volumes, and more.

    Parameters:
    - actor_name: The name of the actor in the level (e.g., "DirectionalLight_0")
    - property_name: The name of the property to set. For nested properties like
                     PostProcessVolume settings, use dot notation (e.g., "Settings.ColorSaturation")
    - property_value: The value to set. Format depends on property type:
        - For float/int/bool: Just the value (e.g., 5.0, 100, true)
        - For FLinearColor: {"R": 1.0, "G": 0.5, "B": 0.2, "A": 1.0}
        - For FVector: [X, Y, Z] (e.g., [100.0, 200.0, 300.0])
        - For FVector4: {"X": 1.0, "Y": 1.0, "Z": 1.0, "W": 1.0} or [X, Y, Z, W]
    - component_name: Optional. The specific component to modify. If not provided,
                      the tool will try to find the appropriate default component.

    Supported properties by actor type:

    DirectionalLight / PointLight / SpotLight:
    - LightColor: FLinearColor - The color of the light
    - Intensity: float - Light intensity (lux for directional, candelas for others)
    - bCastShadows: bool - Whether the light casts shadows
    - Temperature: float - Color temperature in Kelvin

    ExponentialHeightFog:
    - FogDensity: float - The fog density
    - FogInscatteringColor: FLinearColor - The fog color
    - bEnableVolumetricFog: bool - Enable volumetric fog
    - VolumetricFogScatteringDistribution: float - Scattering distribution

    SkyLight:
    - Intensity: float - Sky light intensity
    - LightColor: FLinearColor - Sky light color

    PostProcessVolume (use "Settings." prefix):
    - Settings.bUnbound: bool - Whether the volume affects the entire world
    - Settings.ColorSaturation: FVector4 - Color saturation (XYZW = RGBA multipliers)
    - Settings.ColorContrast: FVector4 - Color contrast
    - Settings.ColorGain: FVector4 - Color gain
    - Settings.ColorGamma: FVector4 - Color gamma correction
    - Settings.FilmWhitePoint: FLinearColor - White point color
    - Settings.BloomIntensity: float - Bloom effect intensity
    - Settings.AutoExposureBias: float - Exposure compensation

    Returns:
        Dictionary with success status and details about the property that was set.

    Example usage:
        # Set directional light color to warm sunset
        set_actor_property("DirectionalLight_0", "LightColor",
                          {"R": 1.0, "G": 0.6, "B": 0.3, "A": 1.0})

        # Set fog density
        set_actor_property("ExponentialHeightFog_0", "FogDensity", 0.02)

        # Set post-process color saturation
        set_actor_property("PostProcessVolume_0", "Settings.ColorSaturation",
                          {"X": 1.2, "Y": 1.0, "Z": 0.8, "W": 1.0})
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "actor_name": actor_name,
            "property_name": property_name,
            "property_value": property_value
        }
        if component_name:
            params["component_name"] = component_name

        response = unreal.send_command("set_actor_property", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"set_actor_property error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def get_actor_properties(
    actor_name: str,
    component_name: str = ""
) -> Dict[str, Any]:
    """
    Get information about an actor's components and their editable properties.

    This is useful for discovering what properties can be modified on an actor
    before using set_actor_property.

    Parameters:
    - actor_name: The name of the actor in the level
    - component_name: Optional. Filter to show only a specific component's properties

    Returns:
        Dictionary containing:
        - actor: The actor name
        - actor_class: The actor's class name
        - components: List of components, each with:
            - name: Component name
            - class: Component class name
            - properties: List of editable properties with name and type
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {"actor_name": actor_name}
        if component_name:
            params["component_name"] = component_name

        response = unreal.send_command("get_actor_properties", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"get_actor_properties error: {e}")
        return {"success": False, "message": str(e)}



# Material Creation and Management Tools
@mcp.tool()
def create_material(
    name: str,
    path: str = "/Game/Materials/",
    base_color: List[float] = None,
    roughness: float = 0.5,
    metallic: float = 0.0
) -> Dict[str, Any]:
    """
    Create a new material asset with basic PBR properties.

    Parameters:
    - name: The name of the material (e.g., "M_EarthTone")
    - path: The content browser path where the material will be created (default: "/Game/Materials/")
    - base_color: RGB or RGBA color values as a list [R, G, B] or [R, G, B, A], values 0.0-1.0
    - roughness: Surface roughness (0.0 = smooth/shiny, 1.0 = rough/matte)
    - metallic: Metallic property (0.0 = non-metal, 1.0 = full metal)

    Returns:
        Dictionary with success status and the created material's path.

    Example usage:
        # Create an earth-tone material
        create_material("M_EarthTone", base_color=[0.18, 0.11, 0.07], roughness=0.8)

        # Create a shiny metal material
        create_material("M_ShinyMetal", base_color=[0.9, 0.9, 0.9], roughness=0.1, metallic=1.0)
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "name": name,
            "path": path,
            "roughness": roughness,
            "metallic": metallic
        }
        if base_color:
            params["base_color"] = base_color

        response = unreal.send_command("create_material", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"create_material error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def create_material_instance(
    name: str,
    parent_material: str,
    path: str = "",
    scalar_parameters: Dict[str, float] = None,
    vector_parameters: Dict[str, List[float]] = None
) -> Dict[str, Any]:
    """
    Create a material instance from an existing parent material.

    Material instances allow you to create variations of a material without
    duplicating the shader. You can override exposed parameters from the parent.

    Parameters:
    - name: The name of the material instance (e.g., "MI_EarthTone_Variant")
    - parent_material: Path to the parent material (e.g., "/Game/Materials/M_EarthTone")
    - path: Optional destination path (defaults to same folder as parent)
    - scalar_parameters: Dict of scalar parameter names and values (e.g., {"Roughness": 0.5})
    - vector_parameters: Dict of vector parameter names and RGBA values
                         (e.g., {"BaseColor": [0.2, 0.15, 0.1, 1.0]})

    Returns:
        Dictionary with success status and the created material instance path.

    Example usage:
        # Create a variant with different roughness
        create_material_instance(
            "MI_EarthTone_Smooth",
            "/Game/Materials/M_EarthTone",
            scalar_parameters={"Roughness": 0.2}
        )

        # Create a variant with different color
        create_material_instance(
            "MI_EarthTone_Red",
            "/Game/Materials/M_EarthTone",
            vector_parameters={"BaseColor": [0.8, 0.2, 0.1, 1.0]}
        )
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "name": name,
            "parent_material": parent_material
        }
        if path:
            params["path"] = path
        if scalar_parameters:
            params["scalar_parameters"] = scalar_parameters
        if vector_parameters:
            params["vector_parameters"] = vector_parameters

        response = unreal.send_command("create_material_instance", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"create_material_instance error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def set_material_instance_parameter(
    material_path: str,
    parameter_name: str,
    parameter_value: Any
) -> Dict[str, Any]:
    """
    Set a parameter value on an existing material instance.

    Parameters:
    - material_path: Path to the material instance (e.g., "/Game/Materials/MI_EarthTone")
    - parameter_name: Name of the parameter to modify
    - parameter_value: The new value. Type determines parameter type:
        - float/int: Sets a scalar parameter
        - [R, G, B] or [R, G, B, A]: Sets a vector parameter
        - string (texture path): Sets a texture parameter

    Returns:
        Dictionary with success status and parameter info.

    Example usage:
        # Set a scalar parameter
        set_material_instance_parameter("/Game/Materials/MI_Earth", "Roughness", 0.3)

        # Set a vector (color) parameter
        set_material_instance_parameter("/Game/Materials/MI_Earth", "BaseColor", [0.5, 0.3, 0.2, 1.0])

        # Set a texture parameter
        set_material_instance_parameter("/Game/Materials/MI_Earth", "DiffuseTexture", "/Game/Textures/T_Rock")
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "material_path": material_path,
            "parameter_name": parameter_name,
            "parameter_value": parameter_value
        }

        response = unreal.send_command("set_material_instance_parameter", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"set_material_instance_parameter error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def import_texture(
    source_path: str,
    texture_name: str = "",
    destination_path: str = "/Game/Textures/",
    compression_type: str = "",
    srgb: bool = None,
    flip_green_channel: bool = None
) -> Dict[str, Any]:
    """
    Import a texture file from disk into the Unreal project.

    Supports common image formats: PNG, JPG, TGA, BMP, EXR, HDR

    Parameters:
    - source_path: Full filesystem path to the source image file
    - texture_name: Name for the imported texture (defaults to source filename)
    - destination_path: Content browser path for the texture (default: "/Game/Textures/")
    - compression_type: Texture compression type. Options:
        "Default" (TC_Default) - Standard color textures
        "Normalmap" (TC_Normalmap) - Normal maps (auto disables sRGB)
        "Masks" (TC_Masks) - Channel-packed masks like ARM (linear, no sRGB)
        "Grayscale" (TC_Grayscale) - Single channel textures
        "HDR" (TC_HDR) - High dynamic range textures
        "EditorIcon" (TC_EditorIcon) - UI textures (no compression, no streaming, no mipmaps)
    - srgb: Whether texture uses sRGB color space (True for diffuse, False for normal/ARM/data)
    - flip_green_channel: Flip green channel for OpenGL→DirectX normal map conversion

    Returns:
        Dictionary with success status, texture path, and dimensions.

    Example usage:
        # Import a diffuse texture (default sRGB)
        import_texture("/home/user/textures/rock_diffuse.png", "T_Rock_D", "/Game/Textures/Rocks/")

        # Import a normal map with correct compression
        import_texture("/home/user/textures/rock_normal.png", "T_Rock_N", "/Game/Textures/Rocks/",
                       compression_type="Normalmap", srgb=False, flip_green_channel=True)

        # Import an ARM packed texture (AO/Roughness/Metallic)
        import_texture("/home/user/textures/rock_arm.png", "T_Rock_ARM", "/Game/Textures/Rocks/",
                       compression_type="Masks", srgb=False)

        # Import a UI texture with alpha (health bar, icons)
        import_texture("/home/user/ui/health_bar.png", "T_HealthBar", "/Game/UI/Textures/",
                       compression_type="EditorIcon")
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "source_path": source_path,
            "destination_path": destination_path
        }
        if texture_name:
            params["texture_name"] = texture_name
        if compression_type:
            params["compression_type"] = compression_type
        if srgb is not None:
            params["srgb"] = srgb
        if flip_green_channel is not None:
            params["flip_green_channel"] = flip_green_channel

        response = unreal.send_command("import_texture", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"import_texture error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def import_sound(
    source_path: str,
    sound_name: str = "",
    destination_path: str = "/Game/Audio/",
    looping: bool = False,
    volume: float = 1.0
) -> Dict[str, Any]:
    """
    Import a sound file from disk into the Unreal project.

    Supports WAV and OGG audio formats.

    Parameters:
    - source_path: Full filesystem path to the source audio file (WAV or OGG)
    - sound_name: Name for the imported sound (defaults to source filename)
    - destination_path: Content browser path for the sound (default: "/Game/Audio/")
    - looping: Whether the sound should loop (default: False)
    - volume: Volume multiplier (default: 1.0)

    Returns:
        Dictionary with success status, sound path, duration, sample rate, and channel count.

    Example usage:
        # Import ambient wind sound
        import_sound("/home/user/audio/wind_ambient.wav", "S_Wind_Ambient", "/Game/Audio/Ambient/")

        # Import looping background music
        import_sound("/home/user/audio/combat_music.ogg", "S_Combat_Music", "/Game/Audio/Music/",
                     looping=True, volume=0.5)
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "source_path": source_path,
            "destination_path": destination_path
        }
        if sound_name:
            params["sound_name"] = sound_name
        if looping:
            params["looping"] = looping
        if volume != 1.0:
            params["volume"] = volume

        response = unreal.send_command("import_sound", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"import_sound error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def add_anim_notify(
    animation_path: str,
    time_seconds: float,
    sound_path: str,
    volume: float = 1.0,
    clear_existing: bool = False
) -> Dict[str, Any]:
    """
    Add a PlaySound AnimNotify to an animation at a specific time.

    This is the professional way to sync sounds to animations. The notify fires
    at the exact frame during playback — frame-perfect synchronization.
    Works automatically with dynamic montages (PlaySlotAnimationAsDynamicMontage).

    Parameters:
    - animation_path: Content browser path to the UAnimSequence
    - time_seconds: Time in seconds when the notify should fire
    - sound_path: Content browser path to the USoundBase to play
    - volume: Volume multiplier (default: 1.0)
    - clear_existing: If true, removes all existing notifies before adding (default: False)

    Returns:
        Dictionary with success status, animation length, and total notify count.

    Example usage:
        add_anim_notify("/Game/Characters/MyChar/Animations/Attack",
                        0.3, "/Game/Audio/SFX/S_Hit")
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "animation_path": animation_path,
            "time_seconds": time_seconds,
            "sound_path": sound_path,
            "volume": volume,
            "clear_existing": clear_existing
        }
        response = unreal.send_command("add_anim_notify", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"add_anim_notify error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def get_editor_log(
    num_lines: int = 200,
    filter: str = ""
) -> Dict[str, Any]:
    """
    Read lines from the Unreal Editor log file.

    Reads the current session's log file and returns the last N lines,
    optionally filtered by a substring match. Useful for verifying plugin
    load messages, checking for errors, and debugging runtime issues.

    Parameters:
    - num_lines: Number of lines to return (default: 200, max: 5000)
    - filter: Optional substring filter — only lines containing this string are returned

    Returns:
        Dictionary with log_file path, total_lines, returned_lines count, and lines (newline-joined string).

    Example usage:
        get_editor_log(filter="BELL SKELETON FIX")
        get_editor_log(filter="BUILD_ID", num_lines=50)
        get_editor_log(filter="Error", num_lines=500)
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "num_lines": num_lines,
        }
        if filter:
            params["filter"] = filter
        response = unreal.send_command("get_editor_log", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"get_editor_log error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def set_texture_properties(
    texture_path: str,
    compression_type: str = "",
    srgb: bool = None,
    flip_green_channel: bool = None,
    never_stream: bool = None
) -> Dict[str, Any]:
    """
    Set properties on an existing texture asset in the Unreal project.

    Use this to fix texture compression, sRGB, and normal map settings on
    already-imported textures without reimporting.

    Parameters:
    - texture_path: Content browser path to the texture (e.g., "/Game/Textures/Rocks/T_Rock_N")
    - compression_type: Texture compression type. Options:
        "Default" (TC_Default) - Standard color textures
        "Normalmap" (TC_Normalmap) - Normal maps (linear, special BC5 compression)
        "Masks" (TC_Masks) - Channel-packed masks like ARM (linear, no sRGB)
        "Grayscale" (TC_Grayscale) - Single channel textures
        "HDR" (TC_HDR) - High dynamic range textures
        "EditorIcon" (TC_EditorIcon) - UI textures (no compression, no streaming, no mipmaps)
    - srgb: Whether texture uses sRGB color space (True for diffuse, False for normal/ARM/data)
    - flip_green_channel: Flip green channel for OpenGL to DirectX normal map conversion
    - never_stream: Disable texture streaming (True for UI textures that must be fully resident)

    Returns:
        Dictionary with success status and current texture properties.

    Example usage:
        # Fix a normal map that was imported with wrong settings
        set_texture_properties("/Game/Textures/Rocks/T_Rock_N",
                              compression_type="Normalmap", srgb=False, flip_green_channel=True)

        # Fix an ARM texture to use Masks compression
        set_texture_properties("/Game/Textures/Rocks/T_Rock_ARM",
                              compression_type="Masks", srgb=False)

        # Set a diffuse texture back to default
        set_texture_properties("/Game/Textures/T_Color", compression_type="Default", srgb=True)

        # Convert to UI texture (full quality, no streaming)
        set_texture_properties("/Game/UI/Textures/T_Icon", compression_type="EditorIcon")
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {"texture_path": texture_path}
        if compression_type:
            params["compression_type"] = compression_type
        if srgb is not None:
            params["srgb"] = srgb
        if flip_green_channel is not None:
            params["flip_green_channel"] = flip_green_channel
        if never_stream is not None:
            params["never_stream"] = never_stream

        response = unreal.send_command("set_texture_properties", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"set_texture_properties error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def create_pbr_material(
    name: str,
    path: str = "/Game/Materials/",
    diffuse_texture: str = "",
    normal_texture: str = "",
    arm_texture: str = "",
    roughness_texture: str = "",
    metallic_texture: str = "",
    ao_texture: str = "",
    roughness_value: float = None,
    metallic_value: float = None,
    two_sided: bool = False,
    opacity_mask_texture: str = "",
    blend_mode: str = ""
) -> Dict[str, Any]:
    """
    Create a complete PBR material with correct texture sampler types in one shot.

    Creates a material with properly configured TextureSample nodes:
    - Diffuse: SamplerType=Color (sRGB)
    - Normal: SamplerType=Normal (linear, BC5 compression)
    - ARM: SamplerType=Masks (linear) with ComponentMask splitting to Roughness/Metallic
      NOTE: AO (R channel) is intentionally NOT connected. ARM textures have AO=0 in UV
      padding which causes dark patches. UE5 Lumen handles ambient occlusion automatically.

    Parameters:
    - name: Material asset name (e.g., "M_Boulder")
    - path: Content browser folder (default: "/Game/Materials/")
    - diffuse_texture: Path to base color texture (e.g., "/Game/Textures/T_Rock_D")
    - normal_texture: Path to normal map texture (must have TC_Normalmap compression)
    - arm_texture: Path to packed ARM texture (R=AO, G=Roughness, B=Metallic)
    - roughness_texture: Path to separate roughness texture (if not using ARM)
    - metallic_texture: Path to separate metallic texture (if not using ARM)
    - ao_texture: Path to separate AO texture (if not using ARM)
    - roughness_value: Scalar roughness (0-1) if no texture
    - metallic_value: Scalar metallic (0-1) if no texture
    - opacity_mask_texture: Path to opacity mask texture (auto-sets Masked blend mode for foliage/grass)
    - blend_mode: Override blend mode: "Opaque", "Masked", "Translucent", "Additive"

    Returns:
        Dictionary with material path and expression count.

    Example:
        # Create PBR material with ARM packed texture
        create_pbr_material("M_Boulder", "/Game/Materials/Rocks/",
                           diffuse_texture="/Game/Textures/Rocks/T_Boulder_D",
                           normal_texture="/Game/Textures/Rocks/T_Boulder_N",
                           arm_texture="/Game/Textures/Rocks/T_Boulder_ARM")
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params: Dict[str, Any] = {"name": name, "path": path}
        if diffuse_texture:
            params["diffuse_texture"] = diffuse_texture
        if normal_texture:
            params["normal_texture"] = normal_texture
        if arm_texture:
            params["arm_texture"] = arm_texture
        if roughness_texture:
            params["roughness_texture"] = roughness_texture
        if metallic_texture:
            params["metallic_texture"] = metallic_texture
        if ao_texture:
            params["ao_texture"] = ao_texture
        if roughness_value is not None:
            params["roughness_value"] = roughness_value
        if metallic_value is not None:
            params["metallic_value"] = metallic_value
        if two_sided:
            params["two_sided"] = True
        if opacity_mask_texture:
            params["opacity_mask_texture"] = opacity_mask_texture
        if blend_mode:
            params["blend_mode"] = blend_mode

        response = unreal.send_command("create_pbr_material", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"create_pbr_material error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def create_landscape_material(
    name: str,
    path: str = "/Game/Materials/",
    rock_d: str = "",
    rock_n: str = "",
    mud_d: str = "",
    mud_n: str = "",
    grass_d: str = "",
    grass_n: str = "",
    mud_detail_d: str = "",
    detail_uv_scale: float = 0.004,
    warp_scale: float = 0.002,
    warp_amount: float = 0.12,
    macro_scale: float = 0.00003,
    macro_strength: float = 0.4,
    slope_sharpness: float = 3.0,
    grass_amount: float = 0.5,
    roughness: float = 0.85,
    mud_amount: float = 0.3,
    puddle_amount: float = 0.2,
    height_blend_strength: float = 0.5,
    puddle_height_bias: float = 1.0,
    rubble_amount: float = 0.3,
    stone_amount: float = 0.2,
) -> Dict[str, Any]:
    """
    Create a complete landscape material v9 with UV noise distortion anti-tiling.

    Builds the entire material graph in C++ with:
    - UV noise distortion + fixed-angle rotation dissolve: warps UVs with noise,
      then samples at original + 37.5deg rotated orientation, dissolve-blends them
    - Height-based layer blending with transition noise for natural boundaries
    - Multi-octave noise for concentrated puddle/mud patches
    - World-Z height bias for valley accumulation of puddles and mud
    - Wet edge darkening around puddle boundaries
    - Distance-based tiling fade (reduces warp at far distances)
    - Slope-based rock/mud blend, noise-based grass overlay
    - Macro brightness variation: very low-frequency noise modulates brightness
    - 11 color-coded comment boxes organizing the graph
    - ~130 expression nodes, 13 texture samplers (of 16 max)

    All nodes created and connected in a single tick. Uses WorldPosition-based
    UVs (not LandscapeLayerCoords) for reliable persistence.

    Parameters:
    - name: Material name (e.g., "M_Landscape_Final")
    - path: Content browser path (default: "/Game/Materials/")
    - rock_d/rock_n: Rock diffuse + normal (slopes)
    - mud_d/mud_n: Mud diffuse + normal (flat areas)
    - grass_d/grass_n: Grass diffuse + normal (overlay)
    - mud_detail_d: Mud/dirt detail diffuse texture for overlay patches (optional)
    - detail_uv_scale: WorldPos UV multiplier for detail textures (default 0.004)
    - warp_scale: Noise scale for UV distortion (default 0.002)
    - warp_amount: UV distortion strength (default 0.12)
    - macro_scale: Noise scale for brightness variation (default 0.00003)
    - macro_strength: Brightness modulation amount 0-1 (default 0.4, MI-editable)
    - slope_sharpness: Power exponent for slope detection (default 3.0, MI-editable)
    - grass_amount: Grass overlay blend amount (default 0.5, MI-editable)
    - roughness: Surface roughness value (default 0.85, MI-editable)
    - mud_amount: Mud/dirt overlay intensity (default 0.3, MI-editable)
    - puddle_amount: Puddle overlay intensity (default 0.2, MI-editable)
    - height_blend_strength: How much texture height affects layer transitions (default 0.5, MI-editable)
    - puddle_height_bias: How strongly puddles/mud prefer low-lying areas (default 1.0, MI-editable)
    - rubble_amount: Rubble patch intensity on flat areas (default 0.3, MI-editable)
    - stone_amount: Stone patch intensity on slopes (default 0.2, MI-editable)

    Returns:
        Dictionary with material path, expression count, and sampler count.

    Example:
        create_landscape_material("M_Landscape",
            rock_d="/Game/Textures/Ground/T_Rocky_Terrain_D",
            rock_n="/Game/Textures/Ground/T_Rocky_Terrain_N",
            mud_d="/Game/Textures/Ground/T_Brown_Mud_D",
            mud_n="/Game/Textures/Ground/T_Brown_Mud_N",
            grass_d="/Game/Textures/Ground/T_Grass_Dry_D",
            grass_n="/Game/Textures/Ground/T_Grass_Dry_N",
            mud_detail_d="/Game/Textures/Ground/T_Sand_D",
            mud_amount=0.3, puddle_amount=0.2)
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params: Dict[str, Any] = {"name": name, "path": path}
        if rock_d:
            params["rock_d"] = rock_d
        if rock_n:
            params["rock_n"] = rock_n
        if mud_d:
            params["mud_d"] = mud_d
        if mud_n:
            params["mud_n"] = mud_n
        if grass_d:
            params["grass_d"] = grass_d
        if grass_n:
            params["grass_n"] = grass_n
        if mud_detail_d:
            params["mud_detail_d"] = mud_detail_d
        params["detail_uv_scale"] = detail_uv_scale
        params["warp_scale"] = warp_scale
        params["warp_amount"] = warp_amount
        params["macro_scale"] = macro_scale
        params["macro_strength"] = macro_strength
        params["slope_sharpness"] = slope_sharpness
        params["grass_amount"] = grass_amount
        params["roughness"] = roughness
        params["mud_amount"] = mud_amount
        params["puddle_amount"] = puddle_amount
        params["height_blend_strength"] = height_blend_strength
        params["puddle_height_bias"] = puddle_height_bias
        params["rubble_amount"] = rubble_amount
        params["stone_amount"] = stone_amount

        response = unreal.send_command("create_landscape_material", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"create_landscape_material error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def take_screenshot(
    file_path: str = "",
    width: int = 960,
    height: int = 540
) -> list:
    """
    Take a screenshot of the active Unreal Editor viewport.

    Uses SceneCapture2D to render the scene off-screen — works even when
    the editor viewport is minimized or has no visible render target.
    Returns the screenshot as an inline image (visible to Claude) plus metadata.

    Parameters:
    - file_path: Where to save the PNG (default: project's Saved/Screenshots/MCP_Screenshot.png)
    - width: Screenshot width in pixels (default: 960, range: 320-3840)
    - height: Screenshot height in pixels (default: 540, range: 240-2160)

    Returns:
        List of MCP content items: TextContent with metadata + ImageContent with the screenshot.
    """
    from mcp.types import ImageContent, TextContent

    unreal = get_unreal_connection()
    if not unreal:
        return [TextContent(type="text", text=json.dumps({"success": False, "message": "Failed to connect to Unreal Engine"}))]

    try:
        params = {"width": width, "height": height}
        if file_path:
            params["file_path"] = file_path

        response = unreal.send_command("take_screenshot", params)
        result = response.get("result", response)

        if not result.get("success"):
            return [TextContent(type="text", text=json.dumps(result))]

        # Build response with inline image
        content_items = []

        # Text metadata
        meta = {
            "success": True,
            "file_path": result.get("file_path", ""),
            "width": result.get("width", 0),
            "height": result.get("height", 0),
            "message": result.get("message", ""),
        }
        content_items.append(TextContent(type="text", text=json.dumps(meta)))

        # Inline image so Claude can see the screenshot
        screenshot_path = result.get("file_path", "")
        if screenshot_path and os.path.isfile(screenshot_path):
            with open(screenshot_path, "rb") as f:
                png_bytes = f.read()
            b64_data = base64.standard_b64encode(png_bytes).decode("ascii")
            content_items.append(ImageContent(type="image", data=b64_data, mimeType="image/png"))

        return content_items
    except Exception as e:
        logger.error(f"take_screenshot error: {e}")
        return [TextContent(type="text", text=json.dumps({"success": False, "message": str(e)}))]


@mcp.tool()
def get_material_info(
    material_path: str
) -> Dict[str, Any]:
    """
    Inspect a material's properties and graph connections for debugging.

    Returns two_sided status, blend mode, shading model, all expressions
    (with texture paths, sampler types, component mask channels),
    and which material outputs are connected.

    Parameters:
    - material_path: Full content browser path (e.g., "/Game/Materials/Rocks/M_Boulder_PBR")

    Returns:
        Dictionary with material properties, expressions list, and connection status.
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        response = unreal.send_command("get_material_info", {"material_path": material_path})
        return response.get("result", response)
    except Exception as e:
        logger.error(f"get_material_info error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def focus_viewport_on_actor(
    actor_name: str,
    distance: float = 500.0
) -> Dict[str, Any]:
    """
    Move the editor viewport camera to focus on a specific actor.

    Calculates camera position based on actor bounds and frames it in view.
    Use with take_screenshot to capture specific actors.

    Parameters:
    - actor_name: Name of the actor to focus on
    - distance: Extra distance from actor bounds (default 500 units)

    Returns:
        Dictionary with camera position.
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {"actor_name": actor_name, "distance": distance}
        response = unreal.send_command("focus_viewport_on_actor", params)
        return response.get("result", response)
    except Exception as e:
        logger.error(f"focus_viewport_on_actor error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def get_texture_info(
    texture_path: str
) -> Dict[str, Any]:
    """
    Get detailed information about a texture asset for debugging.

    Returns dimensions, sRGB, compression type, flip_green_channel,
    mip count, and LOD bias.

    Parameters:
    - texture_path: Full content browser path (e.g., "/Game/Textures/Rocks/T_Boulder_D")

    Returns:
        Dictionary with texture properties.
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        response = unreal.send_command("get_texture_info", {"texture_path": texture_path})
        return response.get("result", response)
    except Exception as e:
        logger.error(f"get_texture_info error: {e}")
        return {"success": False, "message": str(e)}


# Asset Import and Management Tools
@mcp.tool()
def import_mesh(
    source_path: str,
    asset_name: str = "",
    destination_path: str = "/Game/Meshes/",
    import_materials: bool = False,
    import_textures: bool = False,
    generate_collision: bool = True,
    enable_nanite: bool = True,
    combine_meshes: bool = True
) -> Dict[str, Any]:
    """
    Import a mesh file (FBX, OBJ) from disk into the Unreal project as a Static Mesh.

    Parameters:
    - source_path: Full filesystem path to the source mesh file (FBX or OBJ)
    - asset_name: Name for the imported mesh asset (defaults to source filename)
    - destination_path: Content browser path (default: "/Game/Meshes/")
    - import_materials: Whether to import embedded materials (default: False - create manually for art control)
    - import_textures: Whether to import embedded textures (default: False - import separately)
    - generate_collision: Auto-generate collision mesh (default: True)
    - enable_nanite: Enable Nanite virtualized geometry (default: True - recommended for high-poly meshes)
    - combine_meshes: Combine all meshes in FBX into one (default: True)

    Returns:
        Dictionary with success status, asset path, vertex/triangle counts, and material slot info.

    Example usage:
        # Import a rock mesh with Nanite
        import_mesh("/home/user/models/boulder.fbx", "SM_Boulder", "/Game/Meshes/Rocks/")

        # Import with default settings
        import_mesh("/home/user/models/terrain.fbx")
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "source_path": source_path,
            "destination_path": destination_path,
            "import_materials": import_materials,
            "import_textures": import_textures,
            "generate_collision": generate_collision,
            "enable_nanite": enable_nanite,
            "combine_meshes": combine_meshes
        }
        if asset_name:
            params["asset_name"] = asset_name

        response = unreal.send_command("import_mesh", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"import_mesh error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def import_skeletal_mesh(
    source_path: str,
    asset_name: str = "",
    destination_path: str = "/Game/Characters/",
    import_animations: bool = False,
    create_physics_asset: bool = True,
    import_morph_targets: bool = True,
    import_materials: bool = False,
    import_textures: bool = False,
    skeleton_path: str = ""
) -> Dict[str, Any]:
    """
    Import an FBX file as a Skeletal Mesh (character/animated mesh with bones).

    HEAVY OPERATION - allow 5+ seconds between consecutive calls.

    Parameters:
    - source_path: Full filesystem path to the FBX file
    - asset_name: Name for the imported asset (defaults to source filename)
    - destination_path: Content browser path (default: "/Game/Characters/")
    - import_animations: Also import animations embedded in the FBX (default: False)
    - create_physics_asset: Create a physics asset for ragdoll/collision (default: True)
    - import_morph_targets: Import blend shapes/morph targets (default: True)
    - import_materials: Import embedded materials (default: False - create manually)
    - import_textures: Import embedded textures (default: False - import separately)
    - skeleton_path: Reuse an existing skeleton asset path (optional, creates new if empty)

    Returns:
        Dictionary with skeletal_mesh_path, skeleton_path, bone_count, bone_names,
        material_slots, morph_targets, and imported_animations if any.

    Example usage:
        # Import a character mesh
        import_skeletal_mesh("/home/user/models/character.fbx", "SK_Character", "/Game/Characters/Robot/")

        # Import with animations included
        import_skeletal_mesh("/home/user/models/character.fbx", "SK_Character", import_animations=True)
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "source_path": source_path,
            "destination_path": destination_path,
            "import_animations": import_animations,
            "create_physics_asset": create_physics_asset,
            "import_morph_targets": import_morph_targets,
            "import_materials": import_materials,
            "import_textures": import_textures,
        }
        if asset_name:
            params["asset_name"] = asset_name
        if skeleton_path:
            params["skeleton_path"] = skeleton_path

        response = unreal.send_command("import_skeletal_mesh", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"import_skeletal_mesh error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def import_animation(
    source_path: str,
    skeleton_path: str,
    animation_name: str = "",
    destination_path: str = "/Game/Characters/Animations/"
) -> Dict[str, Any]:
    """
    Import animation(s) from an FBX file onto an existing skeleton.

    REQUIRES an existing skeleton - import a skeletal mesh first to create one.
    HEAVY OPERATION - allow 3+ seconds between consecutive calls.

    Parameters:
    - source_path: Full filesystem path to the FBX file containing animation data
    - skeleton_path: Content path to an existing USkeleton asset (REQUIRED)
    - animation_name: Override name for the imported animation (defaults to filename)
    - destination_path: Content browser path (default: "/Game/Characters/Animations/")

    Returns:
        Dictionary with list of imported animations, each containing name, path,
        duration_seconds, num_frames, and rate_scale.

    Example usage:
        # Import animations using skeleton from previous skeletal mesh import
        import_animation(
            "/home/user/anims/walk.fbx",
            "/Game/Characters/Robot/SK_Robot_Skeleton"
        )
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "source_path": source_path,
            "skeleton_path": skeleton_path,
            "destination_path": destination_path,
        }
        if animation_name:
            params["animation_name"] = animation_name

        response = unreal.send_command("import_animation", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"import_animation error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def create_character_blueprint(
    blueprint_name: str,
    blueprint_path: str = "/Game/Characters/",
    skeletal_mesh_path: str = "",
    anim_blueprint_path: str = "",
    capsule_radius: float = 40.0,
    capsule_half_height: float = 90.0,
    max_walk_speed: float = 500.0,
    max_sprint_speed: float = 800.0,
    jump_z_velocity: float = 420.0,
    camera_boom_length: float = 250.0,
    camera_boom_socket_offset_z: float = 150.0
) -> Dict[str, Any]:
    """
    Create a Character Blueprint with third-person camera, movement, and optional skeletal mesh.

    Creates a Blueprint based on ACharacter with:
    - Capsule collision (configurable size)
    - CharacterMovementComponent (configurable speeds)
    - SpringArm (camera boom) with camera lag
    - Camera component attached to spring arm
    - Optional skeletal mesh and animation blueprint assignment

    Parameters:
    - blueprint_name: Name for the blueprint (e.g., "BP_RobotCharacter")
    - blueprint_path: Content browser folder (default: "/Game/Characters/")
    - skeletal_mesh_path: Optional skeletal mesh to assign to the character
    - anim_blueprint_path: Optional AnimBlueprint to drive animations
    - capsule_radius: Collision capsule radius in cm (default: 40)
    - capsule_half_height: Collision capsule half-height in cm (default: 90)
    - max_walk_speed: Walking speed in cm/s (default: 500)
    - max_sprint_speed: Sprint speed stored for reference (default: 800)
    - jump_z_velocity: Jump launch velocity (default: 420)
    - camera_boom_length: Camera distance in cm (default: 250)
    - camera_boom_socket_offset_z: Camera height offset in cm (default: 150)

    Returns:
        Dictionary with blueprint path, generated class, component list, and settings.
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "blueprint_name": blueprint_name,
            "blueprint_path": blueprint_path,
            "capsule_radius": capsule_radius,
            "capsule_half_height": capsule_half_height,
            "max_walk_speed": max_walk_speed,
            "max_sprint_speed": max_sprint_speed,
            "jump_z_velocity": jump_z_velocity,
            "camera_boom_length": camera_boom_length,
            "camera_boom_socket_offset_z": camera_boom_socket_offset_z,
        }
        if skeletal_mesh_path:
            params["skeletal_mesh_path"] = skeletal_mesh_path
        if anim_blueprint_path:
            params["anim_blueprint_path"] = anim_blueprint_path

        response = unreal.send_command("create_character_blueprint", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"create_character_blueprint error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def create_anim_blueprint(
    blueprint_name: str,
    skeleton_path: str,
    blueprint_path: str = "/Game/Characters/",
    preview_mesh_path: str = ""
) -> Dict[str, Any]:
    """
    Create an Animation Blueprint targeting a skeleton.

    REQUIRES an existing skeleton - import a skeletal mesh first to create one.
    The AnimBlueprint is created as an empty shell. State machines and blend logic
    should be configured in the Unreal Editor or via blueprint graph commands.

    Parameters:
    - blueprint_name: Name for the AnimBlueprint (e.g., "ABP_Robot")
    - skeleton_path: Content path to target USkeleton (REQUIRED - crashes without it)
    - blueprint_path: Content browser folder (default: "/Game/Characters/")
    - preview_mesh_path: Optional skeletal mesh for the animation editor preview

    Returns:
        Dictionary with anim_blueprint path, skeleton_path, parent_class, generated_class.

    Example usage:
        create_anim_blueprint(
            "ABP_Robot",
            "/Game/Characters/Robot/SK_Robot_Skeleton",
            "/Game/Characters/Robot/"
        )
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "blueprint_name": blueprint_name,
            "skeleton_path": skeleton_path,
            "blueprint_path": blueprint_path,
        }
        if preview_mesh_path:
            params["preview_mesh_path"] = preview_mesh_path

        response = unreal.send_command("create_anim_blueprint", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"create_anim_blueprint error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def setup_locomotion_state_machine(
    anim_blueprint_path: str,
    idle_animation: str,
    walk_animation: str,
    run_animation: str = "",
    jump_animation: str = "",
    walk_speed_threshold: float = 5.0,
    run_speed_threshold: float = 300.0,
    crossfade_duration: float = 0.2
) -> Dict[str, Any]:
    """
    Set up a complete locomotion state machine in an AnimBlueprint.

    Creates a state machine with Idle/Walk/Run states, speed-based transitions,
    and EventBlueprintUpdateAnimation logic that calculates speed from character
    velocity. One-shot tool that creates a fully functional locomotion system.

    Parameters:
    - anim_blueprint_path: Content path to AnimBlueprint (e.g., "/Game/Characters/Robot/ABP_Robot")
    - idle_animation: Content path to idle AnimSequence
    - walk_animation: Content path to walk AnimSequence
    - run_animation: Optional content path to run AnimSequence (omit for 2-state Idle/Walk)
    - jump_animation: Optional content path to jump AnimSequence
    - walk_speed_threshold: Speed threshold for Idle↔Walk transition (default: 5.0)
    - run_speed_threshold: Speed threshold for Walk↔Run transition (default: 300.0)
    - crossfade_duration: Blend duration between states in seconds (default: 0.2)

    Returns:
        Dictionary with state_count, transition_count, and success status.

    Example:
        setup_locomotion_state_machine(
            anim_blueprint_path="/Game/Characters/Robot/ABP_Robot",
            idle_animation="/Game/Characters/Robot/Animations/Idle",
            walk_animation="/Game/Characters/Robot/Animations/Walking",
            run_animation="/Game/Characters/Robot/Animations/Running",
            jump_animation="/Game/Characters/Robot/Animations/Jump",
            walk_speed_threshold=5.0,
            run_speed_threshold=300.0
        )
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "anim_blueprint_path": anim_blueprint_path,
            "idle_animation": idle_animation,
            "walk_animation": walk_animation,
            "walk_speed_threshold": walk_speed_threshold,
            "run_speed_threshold": run_speed_threshold,
            "crossfade_duration": crossfade_duration,
        }
        if run_animation:
            params["run_animation"] = run_animation
        if jump_animation:
            params["jump_animation"] = jump_animation

        response = unreal.send_command("setup_locomotion_state_machine", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"setup_locomotion_state_machine error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def setup_blendspace_locomotion(
    anim_blueprint_path: str,
    idle_animation: str,
    walk_animation: str,
    max_walk_speed: float = 300.0,
    blendspace_path: str = ""
) -> Dict[str, Any]:
    """
    Set up BlendSpace1D-based locomotion in an AnimBlueprint (replaces state machine).

    Creates a BlendSpace1D asset with idle/walk samples, reparents the AnimBP to
    UEnemyAnimInstance (C++ class with smoothed Speed), and wires the AnimGraph:
    BlendSpacePlayer(Speed) → Slot(DefaultSlot) → Output.

    This is the production-correct approach (matches Lyra/ALS patterns):
    - NO state machine, NO animation resets on speed oscillation
    - Continuous blending between idle and walk based on Speed
    - Speed smoothed in C++ via FInterpTo (NativeUpdateAnimation)
    - Slot node preserved for montage overlays (attacks, hit-react, death)

    Parameters:
    - anim_blueprint_path: Content path to existing AnimBlueprint
    - idle_animation: Content path to idle AnimSequence
    - walk_animation: Content path to walk AnimSequence
    - max_walk_speed: Speed value for full walk blend (default: 300.0)
    - blendspace_path: Optional save path for BS1D asset (auto-derived if empty)

    Returns:
        Dictionary with success, blendspace_path, reparented, speed_wired status.

    Example:
        setup_blendspace_locomotion(
            anim_blueprint_path="/Game/Characters/Enemy/ABP_Enemy",
            idle_animation="/Game/Characters/Enemy/Animations/Idle",
            walk_animation="/Game/Characters/Enemy/Animations/Walk",
            max_walk_speed=300.0
        )
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "anim_blueprint_path": anim_blueprint_path,
            "idle_animation": idle_animation,
            "walk_animation": walk_animation,
            "max_walk_speed": max_walk_speed,
        }
        if blendspace_path:
            params["blendspace_path"] = blendspace_path

        response = unreal.send_command("setup_blendspace_locomotion", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"setup_blendspace_locomotion error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def set_character_properties(
    blueprint_path: str,
    anim_blueprint_path: str = "",
    skeletal_mesh_path: str = "",
    mesh_offset_z: Optional[float] = None,
    capsule_half_height: float = 0,
    capsule_radius: float = 0,
    auto_fit_capsule: bool = False,
    mesh_scale: float = 0,
) -> Dict[str, Any]:
    """
    Update properties on an existing Character Blueprint's CDO (Class Default Object).

    Sets AnimBlueprint, SkeletalMesh, mesh offset, and/or capsule dimensions on the character's
    inherited SkeletalMeshComponent and CapsuleComponent. Use this to assign an AnimBP to a character
    that was created without one, or to resize the collision capsule.

    Parameters:
    - blueprint_path: Content path to Character Blueprint (e.g., "/Game/Characters/Robot/BP_RobotCharacter")
    - anim_blueprint_path: Content path to AnimBlueprint to assign (e.g., "/Game/Characters/Robot/ABP_Robot")
    - skeletal_mesh_path: Content path to SkeletalMesh to assign
    - mesh_offset_z: Z offset for the mesh component (useful for centering in capsule)
    - capsule_half_height: CapsuleComponent half-height (0 means don't change)
    - capsule_radius: CapsuleComponent radius (0 means don't change)
    - auto_fit_capsule: If True, automatically calculates capsule size from skeletal mesh bounds and sets mesh Z offset
    - mesh_scale: Uniform scale for the SkeletalMeshComponent (e.g., 3.0 for 3x size). Processed BEFORE auto_fit_capsule so the capsule fits the scaled mesh. Use this instead of actor scale to keep CharacterMovementComponent working correctly.

    Returns:
        Dictionary with list of changes applied.

    Example:
        set_character_properties(
            blueprint_path="/Game/Characters/Robot/BP_RobotCharacter",
            anim_blueprint_path="/Game/Characters/Robot/ABP_Robot",
            auto_fit_capsule=True
        )
    """
    unreal = get_unreal_connection()
    try:
        params = {"blueprint_path": blueprint_path}
        if anim_blueprint_path:
            params["anim_blueprint_path"] = anim_blueprint_path
        if skeletal_mesh_path:
            params["skeletal_mesh_path"] = skeletal_mesh_path
        if mesh_offset_z is not None:
            params["mesh_offset_z"] = mesh_offset_z
        if capsule_half_height > 0:
            params["capsule_half_height"] = capsule_half_height
        if capsule_radius > 0:
            params["capsule_radius"] = capsule_radius
        if mesh_scale > 0:
            params["mesh_scale"] = mesh_scale
        if auto_fit_capsule:
            params["auto_fit_capsule"] = True

        response = unreal.send_command("set_character_properties", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"set_character_properties error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def set_anim_sequence_root_motion(
    anim_sequence_path: str,
    enable_root_motion: bool,
) -> Dict[str, Any]:
    """
    Enable or disable root motion extraction on an AnimSequence asset.

    Parameters:
    - anim_sequence_path: Content path to AnimSequence (e.g., "/Game/Characters/Enemies/Bell/Animations/Walk")
    - enable_root_motion: True to enable root motion, False for in-place locomotion

    Returns:
        Dictionary with previous and new root motion state.
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "anim_sequence_path": anim_sequence_path,
            "enable_root_motion": enable_root_motion,
        }
        response = unreal.send_command("set_anim_sequence_root_motion", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"set_anim_sequence_root_motion error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def set_anim_state_always_reset_on_entry(
    anim_blueprint_path: str,
    state_name: str,
    always_reset_on_entry: bool,
    state_machine_name: str = "",
) -> Dict[str, Any]:
    """
    Set the "Always Reset on Entry" flag for a state in an AnimBlueprint state machine.

    Parameters:
    - anim_blueprint_path: Content path to AnimBlueprint
    - state_name: State name in the state machine (e.g., "Walk")
    - always_reset_on_entry: True to force state reset on every entry, False to preserve continuity
    - state_machine_name: Optional specific state machine graph name

    Returns:
        Dictionary with state update result and compile status.
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "anim_blueprint_path": anim_blueprint_path,
            "state_name": state_name,
            "always_reset_on_entry": always_reset_on_entry,
        }
        if state_machine_name:
            params["state_machine_name"] = state_machine_name

        response = unreal.send_command("set_anim_state_always_reset_on_entry", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"set_anim_state_always_reset_on_entry error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def set_state_machine_max_transitions_per_frame(
    anim_blueprint_path: str,
    max_transitions_per_frame: int,
    state_machine_name: str = "",
) -> Dict[str, Any]:
    """
    Set MaxTransitionsPerFrame on an AnimBlueprint state machine.

    Parameters:
    - anim_blueprint_path: Content path to AnimBlueprint
    - max_transitions_per_frame: Max transitions allowed in one update tick (commonly 1)
    - state_machine_name: Optional specific state machine graph name

    Returns:
        Dictionary with state machine update result and compile status.
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "anim_blueprint_path": anim_blueprint_path,
            "max_transitions_per_frame": max_transitions_per_frame,
        }
        if state_machine_name:
            params["state_machine_name"] = state_machine_name

        response = unreal.send_command("set_state_machine_max_transitions_per_frame", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"set_state_machine_max_transitions_per_frame error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def auto_fit_capsule(
    blueprint_path: str,
) -> Dict[str, Any]:
    """
    Auto-fit capsule component and mesh Z offset to match the skeletal mesh bounds.

    Uses GetImportedBounds() for accurate mesh geometry sizing. Calculates proper
    capsule half-height, radius, and mesh Z offset so the character stands on ground
    without floating.

    Parameters:
    - blueprint_path: Content path to Character Blueprint (e.g., "/Game/Characters/Enemies/Bell/BP_Bell")

    Returns:
        Dictionary with auto-fit results including capsule dimensions and mesh offset.
    """
    unreal = get_unreal_connection()
    try:
        params = {"blueprint_path": blueprint_path, "auto_fit_capsule": True}
        response = unreal.send_command("set_character_properties", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"auto_fit_capsule error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def add_enhanced_input_action_event(
    blueprint_name: str,
    input_action_path: str,
    pos_x: float = 0.0,
    pos_y: float = 0.0,
) -> Dict[str, Any]:
    """
    Add an Enhanced Input Action event node to a Blueprint's event graph.

    Creates a K2Node_EnhancedInputAction that fires when the specified InputAction
    is triggered. The node has exec output pins (Triggered, Started, Completed, etc.)
    and an ActionValue output pin whose type depends on the InputAction's ValueType.

    Parameters:
    - blueprint_name: Full path to the Blueprint (e.g., "/Game/Characters/Robot/BP_RobotCharacter")
    - input_action_path: Path to the InputAction asset (e.g., "/Game/Input/Actions/IA_Move")
    - pos_x: X position in the graph (default 0)
    - pos_y: Y position in the graph (default 0)

    Returns node_id, output_pins list, and whether an existing node was reused.
    """
    unreal = get_unreal_connection()
    response = unreal.send_command("add_enhanced_input_action_event", {
        "blueprint_name": blueprint_name,
        "input_action_path": input_action_path,
        "pos_x": pos_x,
        "pos_y": pos_y,
    })
    return response.get("result", response)


@mcp.tool()
async def create_input_action(
    action_name: str,
    value_type: str = "Bool",
    action_path: str = "/Game/Input/Actions/"
) -> str:
    """
    Create an Enhanced Input Action asset.

    Creates a UInputAction asset that can be bound to keys in an Input Mapping Context
    and listened for in Blueprints via EnhancedInputAction event nodes.

    Parameters:
    - action_name: Name for the input action (e.g., "IA_Sprint", "IA_Attack")
    - value_type: Type of input value:
        "Bool" - Digital on/off (buttons, keys)
        "Axis1D" / "Float" - Single axis (mouse wheel, triggers)
        "Axis2D" / "Vector2D" - Two axes (thumbstick, WASD)
        "Axis3D" / "Vector3D" - Three axes (motion controller)
    - action_path: Content browser folder (default: "/Game/Input/Actions/")

    Returns:
        Dictionary with action_name, action_path, value_type, and whether it already existed.

    Example:
        create_input_action("IA_Sprint", "Bool")
        create_input_action("IA_Attack", "Bool", "/Game/Input/Actions/Combat/")
    """
    unreal = get_unreal_connection()
    try:
        result = unreal.send_command("create_input_action", {
            "action_name": action_name,
            "value_type": value_type,
            "action_path": action_path
        })
        return json.dumps(result, indent=2)
    except Exception as e:
        return json.dumps({"error": str(e)})


@mcp.tool()
async def add_input_mapping(
    context_path: str,
    action_path: str,
    key: str,
    negate: bool = False,
    swizzle: bool = False,
    trigger: str = ""
) -> str:
    """
    Add a key binding to an Input Mapping Context.

    Maps a keyboard/mouse/gamepad key to an Input Action within an existing
    Input Mapping Context. Supports modifiers (negate, swizzle) and trigger types.

    Parameters:
    - context_path: Content path to the Input Mapping Context (e.g., "/Game/Input/IMC_Default")
    - action_path: Content path to the Input Action (e.g., "/Game/Input/Actions/IA_Sprint")
    - key: Unreal key name. Common keys:
        Letters: A, B, C, ... Z
        Numbers: Zero, One, Two, ... Nine
        Arrow keys: Up, Down, Left, Right
        Special: SpaceBar, LeftShift, RightShift, LeftControl, RightControl
        Function: F1-F12
        Numpad: NumPadZero-NumPadNine
        Mouse: LeftMouseButton, RightMouseButton, MiddleMouseButton, MouseScrollUp, MouseScrollDown
        Navigation: Insert, Delete, Home, End, PageUp, PageDown
        Gamepad: Gamepad_LeftX, Gamepad_FaceButton_Bottom, etc.
    - negate: Add Negate modifier (inverts value, useful for opposite directions)
    - swizzle: Add Swizzle YXZ modifier (swaps X and Y axes)
    - trigger: Optional trigger type:
        "" (empty) - Default: fires on Down (every frame while held)
        "Pressed" - Fires once when pressed
        "Released" - Fires once when released
        "Hold" - Fires after held for a duration

    Returns:
        Dictionary with context_path, action_path, key, and applied modifiers.

    Example:
        add_input_mapping("/Game/Input/IMC_Default", "/Game/Input/Actions/IA_Sprint", "LeftShift")
        add_input_mapping("/Game/Input/IMC_Default", "/Game/Input/Actions/IA_Attack", "Insert", trigger="Pressed")
    """
    unreal = get_unreal_connection()
    try:
        params = {
            "context_path": context_path,
            "action_path": action_path,
            "key": key,
            "negate": negate,
            "swizzle": swizzle
        }
        if trigger:
            params["trigger"] = trigger
        result = unreal.send_command("add_input_mapping", params)
        return json.dumps(result, indent=2)
    except Exception as e:
        return json.dumps({"error": str(e)})


@mcp.tool()
def list_assets(
    path: str = "/Game/",
    asset_type: str = "",
    recursive: bool = True
) -> Dict[str, Any]:
    """
    List assets in the Unreal content browser.

    Parameters:
    - path: Content browser path to search (default: "/Game/")
    - asset_type: Filter by asset type: "StaticMesh", "Texture2D", "Material", "SkeletalMesh", "MaterialInstanceConstant" (empty = all types)
    - recursive: Search subdirectories (default: True)

    Returns:
        Dictionary with list of assets, each containing name, path, and class.

    Example usage:
        # List all meshes in the project
        list_assets("/Game/Meshes/", "StaticMesh")

        # List all assets in a folder
        list_assets("/Game/Textures/")
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "path": path,
            "asset_type": asset_type,
            "recursive": recursive
        }

        response = unreal.send_command("list_assets", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"list_assets error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def does_asset_exist(
    asset_path: str
) -> Dict[str, Any]:
    """
    Check if an asset exists in the Unreal content browser.

    Parameters:
    - asset_path: Full content browser path to check (e.g., "/Game/Meshes/SM_Boulder")

    Returns:
        Dictionary with exists (bool) and asset_class if it exists.

    Example usage:
        does_asset_exist("/Game/Meshes/SM_Boulder")
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "asset_path": asset_path
        }

        response = unreal.send_command("does_asset_exist", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"does_asset_exist error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def get_asset_info(
    asset_path: str
) -> Dict[str, Any]:
    """
    Get detailed information about an asset in the Unreal content browser.

    For StaticMesh: returns vertex count, triangle count, bounds, material slots, nanite status.
    For Texture2D: returns dimensions and format.

    Parameters:
    - asset_path: Full content browser path (e.g., "/Game/Meshes/SM_Boulder")

    Returns:
        Dictionary with asset details appropriate to its type.

    Example usage:
        get_asset_info("/Game/Meshes/SM_Boulder")
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "asset_path": asset_path
        }

        response = unreal.send_command("get_asset_info", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"get_asset_info error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def delete_asset(
    asset_path: str,
    check_references: bool = True,
    force_delete: bool = False
) -> Dict[str, Any]:
    """
    Delete an asset from the Unreal content browser.

    Safely removes a material, texture, mesh, or any other asset. By default, checks
    for references from other assets and refuses to delete if references exist (to prevent
    breaking dependencies). Use force_delete=True to override.

    Parameters:
    - asset_path: Full content browser path (e.g., "/Game/Materials/MI_Landscape_Ground")
    - check_references: Check if other assets reference this one before deleting (default: True)
    - force_delete: Delete even if other assets reference this one (default: False)

    Returns:
        Dictionary with deletion result, or error with list of referencers if blocked.

    Example usage:
        # Safe delete (will fail if referenced):
        delete_asset("/Game/Materials/OldMaterial")

        # Force delete regardless of references:
        delete_asset("/Game/Materials/OldMaterial", force_delete=True)
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "asset_path": asset_path,
            "check_references": check_references,
            "force_delete": force_delete
        }

        response = unreal.send_command("delete_asset", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"delete_asset error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def set_nanite_enabled(
    mesh_path: str,
    enabled: bool = False
) -> Dict[str, Any]:
    """
    Enable or disable Nanite on a static mesh asset.

    Nanite is UE5's virtualized geometry system. Disabling it can fix dark
    self-shadowing artifacts caused by Lumen surface cache mismatch on
    concave geometry. Small meshes (<50K tris) often don't need Nanite.

    Parameters:
    - mesh_path: Content browser path to the static mesh (e.g., "/Game/Meshes/Rocks/SM_Boulder_01")
    - enabled: True to enable Nanite, False to disable (default: False)

    Returns:
        Dictionary with success status and previous/new Nanite state.
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "mesh_path": mesh_path,
            "enabled": enabled
        }

        response = unreal.send_command("set_nanite_enabled", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"set_nanite_enabled error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def get_height_at_location(
    x: float,
    y: float
) -> Dict[str, Any]:
    """
    Query the terrain/landscape surface height at a given world XY position.

    Performs a line trace from high above (Z=100000) straight down to find the first
    surface hit. Returns the Z height, surface normal, and the name of the hit actor.

    Parameters:
    - x: World X coordinate (Unreal units)
    - y: World Y coordinate (Unreal units)

    Returns:
        Dictionary with x, y, z of the hit point, surface normal, and hit actor name.

    Example usage:
        get_height_at_location(500.0, -1200.0)
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {"x": x, "y": y}
        response = unreal.send_command("get_height_at_location", params)
        return response.get("result", response)
    except Exception as e:
        logger.error(f"get_height_at_location error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def snap_actor_to_ground(
    actor_name: str
) -> Dict[str, Any]:
    """
    Snap an existing actor to the ground/landscape surface directly below it.

    Finds the actor by name, performs a downward line trace from its XY position,
    and moves it to sit on the surface. The actor's XY position and rotation are preserved.

    Parameters:
    - actor_name: The exact name of the actor in the level (e.g., "Rock_Boulder_01")

    Returns:
        Dictionary with old_z, new_z, surface_z, and the name of the surface hit.

    Example usage:
        snap_actor_to_ground("Rock_Boulder_01")
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {"actor_name": actor_name}
        response = unreal.send_command("snap_actor_to_ground", params)
        return response.get("result", response)
    except Exception as e:
        logger.error(f"snap_actor_to_ground error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def scatter_meshes_on_landscape(
    center: List[float],
    items: List[Dict[str, Any]],
    delete_existing: bool = True,
    random_offset: float = 0.0,
    random_yaw: bool = False,
    random_scale_variance: float = 0.0
) -> Dict[str, Any]:
    """
    Scatter multiple meshes on the landscape surface in a single operation.

    Places StaticMeshActors at positions relative to a center point, automatically
    querying terrain height via line trace so each mesh sits on the ground.

    Parameters:
    - center: [X, Y] center point on the landscape in Unreal world units
    - items: List of dicts, each with:
        - name: Actor name (string)
        - static_mesh: Asset path (e.g., "/Game/Meshes/Rocks/SM_Boulder_01")
        - offset: [dX, dY] offset from center in Unreal units
        - rotation: [Pitch, Yaw, Roll] in degrees (optional, default [0,0,0])
        - scale: [X, Y, Z] scale factors (optional, default [1,1,1])
    - delete_existing: If True, delete any existing actors with the same names first (default True)
    - random_offset: Random XY position jitter in Unreal units (±range added to each offset)
    - random_yaw: If True, each item gets a random yaw (0-360°) plus slight pitch/roll tilt
    - random_scale_variance: Random scale variation as fraction (e.g., 0.2 = ±20% of specified scale)

    Returns:
        Dictionary with placed_count, actors array (with position details), and any errors.

    Example usage:
        scatter_meshes_on_landscape(
            center=[-6000, 8000],
            items=[
                {"name": "Rock_01", "static_mesh": "/Game/Meshes/Rocks/SM_Boulder_01",
                 "offset": [0, 0], "scale": [2, 2, 2]},
                {"name": "Rock_02", "static_mesh": "/Game/Meshes/Rocks/SM_Moss_01",
                 "offset": [300, -100], "scale": [10, 10, 10]},
            ],
            random_offset=200,
            random_yaw=True,
            random_scale_variance=0.15
        )
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "center": center,
            "items": items,
            "delete_existing": delete_existing,
            "random_offset": random_offset,
            "random_yaw": random_yaw,
            "random_scale_variance": random_scale_variance
        }
        response = unreal.send_command("scatter_meshes_on_landscape", params)
        return response.get("result", response)
    except Exception as e:
        logger.error(f"scatter_meshes_on_landscape error: {e}")
        return {"success": False, "message": str(e)}


# Essential Blueprint Tools for Physics Actors
@mcp.tool()
def create_blueprint(name: str, parent_class: str) -> Dict[str, Any]:
    """Create a new Blueprint class."""
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}
    
    try:
        params = {
            "name": name,
            "parent_class": parent_class
        }
        response = unreal.send_command("create_blueprint", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"create_blueprint error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def add_component_to_blueprint(
    blueprint_name: str,
    component_type: str,
    component_name: str,
    location: List[float] = [],
    rotation: List[float] = [],
    scale: List[float] = [],
    component_properties: Dict[str, Any] = {}
) -> Dict[str, Any]:
    """Add a component to a Blueprint."""
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}
    
    try:
        params = {
            "blueprint_name": blueprint_name,
            "component_type": component_type,
            "component_name": component_name,
            "location": location,
            "rotation": rotation,
            "scale": scale,
            "component_properties": component_properties
        }
        response = unreal.send_command("add_component_to_blueprint", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"add_component_to_blueprint error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def set_static_mesh_properties(
    blueprint_name: str,
    component_name: str,
    static_mesh: str = "/Engine/BasicShapes/Cube.Cube"
) -> Dict[str, Any]:
    """Set static mesh properties on a StaticMeshComponent."""
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}
    
    try:
        params = {
            "blueprint_name": blueprint_name,
            "component_name": component_name,
            "static_mesh": static_mesh
        }
        response = unreal.send_command("set_static_mesh_properties", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"set_static_mesh_properties error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def set_physics_properties(
    blueprint_name: str,
    component_name: str,
    simulate_physics: bool = True,
    gravity_enabled: bool = True,
    mass: float = 1,
    linear_damping: float = 0.01,
    angular_damping: float = 0
) -> Dict[str, Any]:
    """Set physics properties on a component."""
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}
    
    try:
        params = {
            "blueprint_name": blueprint_name,
            "component_name": component_name,
            "simulate_physics": simulate_physics,
            "gravity_enabled": gravity_enabled,
            "mass": mass,
            "linear_damping": linear_damping,
            "angular_damping": angular_damping
        }
        response = unreal.send_command("set_physics_properties", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"set_physics_properties error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def compile_blueprint(blueprint_name: str) -> Dict[str, Any]:
    """Compile a Blueprint."""
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}
    
    try:
        params = {"blueprint_name": blueprint_name}
        response = unreal.send_command("compile_blueprint", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"compile_blueprint error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def read_blueprint_content(
    blueprint_path: str,
    include_event_graph: bool = True,
    include_functions: bool = True,
    include_variables: bool = True,
    include_components: bool = True,
    include_interfaces: bool = True
) -> Dict[str, Any]:
    """
    Read and analyze the complete content of a Blueprint including event graph, 
    functions, variables, components, and implemented interfaces.
    
    Args:
        blueprint_path: Full path to the Blueprint asset (e.g., "/Game/MyBlueprint.MyBlueprint")
        include_event_graph: Include event graph nodes and connections
        include_functions: Include custom functions and their graphs
        include_variables: Include all Blueprint variables with types and defaults
        include_components: Include component hierarchy and properties
        include_interfaces: Include implemented Blueprint interfaces
    
    Returns:
        Dictionary containing complete Blueprint structure and content
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}
    
    try:
        params = {
            "blueprint_path": blueprint_path,
            "include_event_graph": include_event_graph,
            "include_functions": include_functions,
            "include_variables": include_variables,
            "include_components": include_components,
            "include_interfaces": include_interfaces
        }
        
        logger.info(f"Reading Blueprint content for: {blueprint_path}")
        response = unreal.send_command("read_blueprint_content", params)
        
        if response and response.get("success", False):
            logger.info(f"Successfully read Blueprint content. Found:")
            if response.get("variables"):
                logger.info(f"  - {len(response['variables'])} variables")
            if response.get("functions"):
                logger.info(f"  - {len(response['functions'])} functions")
            if response.get("event_graph", {}).get("nodes"):
                logger.info(f"  - {len(response['event_graph']['nodes'])} event graph nodes")
            if response.get("components"):
                logger.info(f"  - {len(response['components'])} components")
        
        return response or {"success": False, "message": "No response from Unreal"}
        
    except Exception as e:
        logger.error(f"read_blueprint_content error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def analyze_blueprint_graph(
    blueprint_path: str,
    graph_name: str = "EventGraph",
    include_node_details: bool = True,
    include_pin_connections: bool = True,
    trace_execution_flow: bool = True
) -> Dict[str, Any]:
    """
    Analyze a specific graph within a Blueprint (EventGraph, functions, etc.)
    and provide detailed information about nodes, connections, and execution flow.
    
    Args:
        blueprint_path: Full path to the Blueprint asset
        graph_name: Name of the graph to analyze ("EventGraph", function name, etc.)
        include_node_details: Include detailed node properties and settings
        include_pin_connections: Include all pin-to-pin connections
        trace_execution_flow: Trace the execution flow through the graph
    
    Returns:
        Dictionary with graph analysis including nodes, connections, and flow
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}
    
    try:
        params = {
            "blueprint_path": blueprint_path,
            "graph_name": graph_name,
            "include_node_details": include_node_details,
            "include_pin_connections": include_pin_connections,
            "trace_execution_flow": trace_execution_flow
        }
        
        logger.info(f"Analyzing Blueprint graph: {blueprint_path} -> {graph_name}")
        response = unreal.send_command("analyze_blueprint_graph", params)
        
        if response and response.get("success", False):
            graph_data = response.get("graph_data", {})
            logger.info(f"Graph analysis complete:")
            logger.info(f"  - Graph: {graph_data.get('graph_name', 'Unknown')}")
            logger.info(f"  - Nodes: {len(graph_data.get('nodes', []))}")
            logger.info(f"  - Connections: {len(graph_data.get('connections', []))}")
            if graph_data.get('execution_paths'):
                logger.info(f"  - Execution paths: {len(graph_data['execution_paths'])}")
        
        return response or {"success": False, "message": "No response from Unreal"}
        
    except Exception as e:
        logger.error(f"analyze_blueprint_graph error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def get_blueprint_variable_details(
    blueprint_path: str,
    variable_name: str = None
) -> Dict[str, Any]:
    """
    Get detailed information about Blueprint variables including type, 
    default values, metadata, and usage within the Blueprint.
    
    Args:
        blueprint_path: Full path to the Blueprint asset
        variable_name: Specific variable name (if None, returns all variables)
    
    Returns:
        Dictionary with variable details including type, defaults, and usage
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}
    
    try:
        params = {
            "blueprint_path": blueprint_path,
            "variable_name": variable_name
        }
        
        logger.info(f"Getting Blueprint variable details: {blueprint_path}")
        if variable_name:
            logger.info(f"  - Specific variable: {variable_name}")
        
        response = unreal.send_command("get_blueprint_variable_details", params)
        return response or {"success": False, "message": "No response from Unreal"}
        
    except Exception as e:
        logger.error(f"get_blueprint_variable_details error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def get_blueprint_function_details(
    blueprint_path: str,
    function_name: str = None,
    include_graph: bool = True
) -> Dict[str, Any]:
    """
    Get detailed information about Blueprint functions including parameters,
    return values, local variables, and function graph content.
    
    Args:
        blueprint_path: Full path to the Blueprint asset
        function_name: Specific function name (if None, returns all functions)
        include_graph: Include the function's graph nodes and connections
    
    Returns:
        Dictionary with function details including signature and graph content
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}
    
    try:
        params = {
            "blueprint_path": blueprint_path,
            "function_name": function_name,
            "include_graph": include_graph
        }
        
        logger.info(f"Getting Blueprint function details: {blueprint_path}")
        if function_name:
            logger.info(f"  - Specific function: {function_name}")
        
        response = unreal.send_command("get_blueprint_function_details", params)
        return response or {"success": False, "message": "No response from Unreal"}
        
    except Exception as e:
        logger.error(f"get_blueprint_function_details error: {e}")
        return {"success": False, "message": str(e)}



# Advanced Composition Tools
@mcp.tool()
def create_pyramid(
    base_size: int = 3,
    block_size: float = 100.0,
    location: List[float] = [0.0, 0.0, 0.0],
    name_prefix: str = "PyramidBlock",
    mesh: str = "/Engine/BasicShapes/Cube.Cube"
) -> Dict[str, Any]:
    """Spawn a pyramid made of cube actors."""
    try:
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}
        spawned = []
        scale = block_size / 100.0
        for level in range(base_size):
            count = base_size - level
            for x in range(count):
                for y in range(count):
                    actor_name = f"{name_prefix}_{level}_{x}_{y}"
                    loc = [
                        location[0] + (x - (count - 1)/2) * block_size,
                        location[1] + (y - (count - 1)/2) * block_size,
                        location[2] + level * block_size
                    ]
                    params = {
                        "name": actor_name,
                        "type": "StaticMeshActor",
                        "location": loc,
                        "scale": [scale, scale, scale],
                        "static_mesh": mesh
                    }
                    resp = safe_spawn_actor(unreal, params)
                    if resp and resp.get("status") == "success":
                        spawned.append(resp)
        return {"success": True, "actors": spawned}
    except Exception as e:
        logger.error(f"create_pyramid error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def create_wall(
    length: int = 5,
    height: int = 2,
    block_size: float = 100.0,
    location: List[float] = [0.0, 0.0, 0.0],
    orientation: str = "x",
    name_prefix: str = "WallBlock",
    mesh: str = "/Engine/BasicShapes/Cube.Cube"
) -> Dict[str, Any]:
    """Create a simple wall from cubes."""
    try:
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}
        spawned = []
        scale = block_size / 100.0
        for h in range(height):
            for i in range(length):
                actor_name = f"{name_prefix}_{h}_{i}"
                if orientation == "x":
                    loc = [location[0] + i * block_size, location[1], location[2] + h * block_size]
                else:
                    loc = [location[0], location[1] + i * block_size, location[2] + h * block_size]
                params = {
                    "name": actor_name,
                    "type": "StaticMeshActor",
                    "location": loc,
                    "scale": [scale, scale, scale],
                    "static_mesh": mesh
                }
                resp = safe_spawn_actor(unreal, params)
                if resp and resp.get("status") == "success":
                    spawned.append(resp)
        return {"success": True, "actors": spawned}
    except Exception as e:
        logger.error(f"create_wall error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def create_tower(
    height: int = 10,
    base_size: int = 4,
    block_size: float = 100.0,
    location: List[float] = [0.0, 0.0, 0.0],
    name_prefix: str = "TowerBlock",
    mesh: str = "/Engine/BasicShapes/Cube.Cube",
    tower_style: str = "cylindrical"  # "cylindrical", "square", "tapered"
) -> Dict[str, Any]:
    """Create a realistic tower with various architectural styles."""
    try:
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}
        spawned = []
        scale = block_size / 100.0

        for level in range(height):
            level_height = location[2] + level * block_size
            
            if tower_style == "cylindrical":
                # Create circular tower
                radius = (base_size / 2) * block_size  # Convert to world units (centimeters)
                circumference = 2 * math.pi * radius
                num_blocks = max(8, int(circumference / block_size))
                
                for i in range(num_blocks):
                    angle = (2 * math.pi * i) / num_blocks
                    x = location[0] + radius * math.cos(angle)
                    y = location[1] + radius * math.sin(angle)
                    
                    actor_name = f"{name_prefix}_{level}_{i}"
                    params = {
                        "name": actor_name,
                        "type": "StaticMeshActor",
                        "location": [x, y, level_height],
                        "scale": [scale, scale, scale],
                        "static_mesh": mesh
                    }
                    resp = safe_spawn_actor(unreal, params)
                    if resp and resp.get("status") == "success":
                        spawned.append(resp)
                        
            elif tower_style == "tapered":
                # Create tapering square tower
                current_size = max(1, base_size - (level // 2))
                half_size = current_size / 2
                
                # Create walls for current level
                for side in range(4):
                    for i in range(current_size):
                        if side == 0:  # Front wall
                            x = location[0] + (i - half_size + 0.5) * block_size
                            y = location[1] - half_size * block_size
                            actor_name = f"{name_prefix}_{level}_front_{i}"
                        elif side == 1:  # Right wall
                            x = location[0] + half_size * block_size
                            y = location[1] + (i - half_size + 0.5) * block_size
                            actor_name = f"{name_prefix}_{level}_right_{i}"
                        elif side == 2:  # Back wall
                            x = location[0] + (half_size - i - 0.5) * block_size
                            y = location[1] + half_size * block_size
                            actor_name = f"{name_prefix}_{level}_back_{i}"
                        else:  # Left wall
                            x = location[0] - half_size * block_size
                            y = location[1] + (half_size - i - 0.5) * block_size
                            actor_name = f"{name_prefix}_{level}_left_{i}"
                            
                        params = {
                            "name": actor_name,
                            "type": "StaticMeshActor",
                            "location": [x, y, level_height],
                            "scale": [scale, scale, scale],
                            "static_mesh": mesh
                        }
                        resp = unreal.send_command("spawn_actor", params)
                        if resp:
                            spawned.append(resp)
                            
            else:  # square tower
                # Create square tower walls
                half_size = base_size / 2
                
                # Four walls
                for side in range(4):
                    for i in range(base_size):
                        if side == 0:  # Front wall
                            x = location[0] + (i - half_size + 0.5) * block_size
                            y = location[1] - half_size * block_size
                            actor_name = f"{name_prefix}_{level}_front_{i}"
                        elif side == 1:  # Right wall
                            x = location[0] + half_size * block_size
                            y = location[1] + (i - half_size + 0.5) * block_size
                            actor_name = f"{name_prefix}_{level}_right_{i}"
                        elif side == 2:  # Back wall
                            x = location[0] + (half_size - i - 0.5) * block_size
                            y = location[1] + half_size * block_size
                            actor_name = f"{name_prefix}_{level}_back_{i}"
                        else:  # Left wall
                            x = location[0] - half_size * block_size
                            y = location[1] + (half_size - i - 0.5) * block_size
                            actor_name = f"{name_prefix}_{level}_left_{i}"
                            
                        params = {
                            "name": actor_name,
                            "type": "StaticMeshActor",
                            "location": [x, y, level_height],
                            "scale": [scale, scale, scale],
                            "static_mesh": mesh
                        }
                        resp = unreal.send_command("spawn_actor", params)
                        if resp:
                            spawned.append(resp)
                            
            # Add decorative elements every few levels
            if level % 3 == 2 and level < height - 1:
                # Add corner details
                for corner in range(4):
                    angle = corner * math.pi / 2
                    detail_x = location[0] + (base_size/2 + 0.5) * block_size * math.cos(angle)
                    detail_y = location[1] + (base_size/2 + 0.5) * block_size * math.sin(angle)
                    
                    actor_name = f"{name_prefix}_{level}_detail_{corner}"
                    params = {
                        "name": actor_name,
                        "type": "StaticMeshActor",
                        "location": [detail_x, detail_y, level_height],
                        "scale": [scale * 0.7, scale * 0.7, scale * 0.7],
                        "static_mesh": "/Engine/BasicShapes/Cylinder.Cylinder"
                    }
                    resp = safe_spawn_actor(unreal, params)
                    if resp and resp.get("status") == "success":
                        spawned.append(resp)
                        
        return {"success": True, "actors": spawned, "tower_style": tower_style}
    except Exception as e:
        logger.error(f"create_tower error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def create_staircase(
    steps: int = 5,
    step_size: List[float] = [100.0, 100.0, 50.0],
    location: List[float] = [0.0, 0.0, 0.0],
    name_prefix: str = "Stair",
    mesh: str = "/Engine/BasicShapes/Cube.Cube"
) -> Dict[str, Any]:
    """Create a staircase from cubes."""
    try:
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}
        spawned = []
        sx, sy, sz = step_size
        for i in range(steps):
            actor_name = f"{name_prefix}_{i}"
            loc = [location[0] + i * sx, location[1], location[2] + i * sz]
            scale = [sx/100.0, sy/100.0, sz/100.0]
            params = {
                "name": actor_name,
                "type": "StaticMeshActor",
                "location": loc,
                "scale": scale,
                "static_mesh": mesh
            }
            resp = safe_spawn_actor(unreal, params)
            if resp and resp.get("status") == "success":
                spawned.append(resp)
        return {"success": True, "actors": spawned}
    except Exception as e:
        logger.error(f"create_staircase error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def construct_house(
    width: int = 1200,
    depth: int = 1000,
    height: int = 600,
    location: List[float] = [0.0, 0.0, 0.0],
    name_prefix: str = "House",
    mesh: str = "/Engine/BasicShapes/Cube.Cube",
    house_style: str = "modern"  # "modern", "cottage"
) -> Dict[str, Any]:
    """Construct a realistic house with architectural details and multiple rooms."""
    try:
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        # Use the helper function to build the house
        return build_house(unreal, width, depth, height, location, name_prefix, mesh, house_style)

    except Exception as e:
        logger.error(f"construct_house error: {e}")
        return {"success": False, "message": str(e)}



@mcp.tool()
def construct_mansion(
    mansion_scale: str = "large",  # "small", "large", "epic", "legendary"
    location: List[float] = [0.0, 0.0, 0.0],
    name_prefix: str = "Mansion"
) -> Dict[str, Any]:
    """
    Construct a magnificent mansion with multiple wings, grand rooms, gardens,
    fountains, and luxury features perfect for dramatic TikTok reveals.
    """
    try:
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        logger.info(f"Creating {mansion_scale} mansion")
        all_actors = []

        # Get size parameters and calculate scaled dimensions
        params = get_mansion_size_params(mansion_scale)
        layout = calculate_mansion_layout(params)

        # Build mansion main structure
        build_mansion_main_structure(unreal, name_prefix, location, layout, all_actors)

        # Build mansion exterior
        build_mansion_exterior(unreal, name_prefix, location, layout, all_actors)

        # Add luxurious interior
        add_mansion_interior(unreal, name_prefix, location, layout, all_actors)

        logger.info(f"Mansion construction complete! Created {len(all_actors)} elements")

        return {
            "success": True,
            "message": f"Magnificent {mansion_scale} mansion created with {len(all_actors)} elements!",
            "actors": all_actors,
            "stats": {
                "scale": mansion_scale,
                "wings": layout["wings"],
                "floors": layout["floors"],
                "main_rooms": layout["main_rooms"],
                "bedrooms": layout["bedrooms"],
                "garden_size": layout["garden_size"],
                "fountain_count": layout["fountain_count"],
                "car_count": layout["car_count"],
                "total_actors": len(all_actors)
            }
        }

    except Exception as e:
        logger.error(f"construct_mansion error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def create_arch(
    radius: float = 300.0,
    segments: int = 6,
    location: List[float] = [0.0, 0.0, 0.0],
    name_prefix: str = "ArchBlock",
    mesh: str = "/Engine/BasicShapes/Cube.Cube"
) -> Dict[str, Any]:
    """Create a simple arch using cubes in a semicircle."""
    try:
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}
        spawned = []
        angle_step = math.pi / segments
        scale = radius / 300.0 / 2
        for i in range(segments + 1):
            theta = angle_step * i
            x = radius * math.cos(theta)
            z = radius * math.sin(theta)
            actor_name = f"{name_prefix}_{i}"
            params = {
                "name": actor_name,
                "type": "StaticMeshActor",
                "location": [location[0] + x, location[1], location[2] + z],
                "scale": [scale, scale, scale],
                "static_mesh": mesh
            }
            resp = safe_spawn_actor(unreal, params)
            if resp and resp.get("status") == "success":
                spawned.append(resp)
        return {"success": True, "actors": spawned}
    except Exception as e:
        logger.error(f"create_arch error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def spawn_actor(
    name: str,
    type: str = "StaticMeshActor",
    static_mesh: str = "",
    location: List[float] = [0.0, 0.0, 0.0],
    rotation: List[float] = [0.0, 0.0, 0.0],
    scale: List[float] = [1.0, 1.0, 1.0],
    sound_asset: str = "",
    volume_multiplier: float = 1.0,
    pitch_multiplier: float = 1.0,
    auto_activate: bool = True,
    is_ui_sound: bool = False,
    attenuation_max_distance: float = 0.0,
) -> dict:
    """
    Spawn an actor in the level.

    Parameters:
    - name: Unique name for the actor
    - type: Actor type - "StaticMeshActor", "PointLight", "SpotLight",
            "DirectionalLight", "CameraActor", "CineCameraActor",
            "ExponentialHeightFog", "SkyLight", "PostProcessVolume", "DecalActor",
            "AmbientSound" - Spatialized or 2D audio source. Extra params:
                sound_asset (str): Path to USoundWave or USoundCue (e.g., "/Game/Audio/S_Wind")
                volume_multiplier (float): Volume scale, default 1.0
                pitch_multiplier (float): Pitch scale, default 1.0
                auto_activate (bool): Start playing automatically, default true
                is_ui_sound (bool): If true, non-spatialized 2D audio (for music), default false
                attenuation_max_distance (float): Max hearing distance in units
    - static_mesh: For StaticMeshActor, the mesh asset path (e.g. "/Game/Meshes/Rocks/SM_Boulder_01")
    - location: [X, Y, Z] position in Unreal units
    - rotation: [Pitch, Yaw, Roll] in degrees
    - scale: [X, Y, Z] scale factors

    Returns:
        Dictionary with spawned actor name, class, location, rotation, scale.

    Example:
        spawn_actor("MyRock", "StaticMeshActor", "/Game/Meshes/Rocks/SM_Boulder_01",
                    location=[100, 200, 0], rotation=[0, 45, 0], scale=[2, 2, 2])
    """
    unreal = get_unreal_connection()
    params = {
        "name": name,
        "type": type,
        "location": location,
        "rotation": rotation,
        "scale": scale,
    }
    if static_mesh:
        params["static_mesh"] = static_mesh
    # AmbientSound-specific parameters
    if sound_asset:
        params["sound_asset"] = sound_asset
    if volume_multiplier != 1.0:
        params["volume_multiplier"] = volume_multiplier
    if pitch_multiplier != 1.0:
        params["pitch_multiplier"] = pitch_multiplier
    if not auto_activate:
        params["auto_activate"] = False
    if is_ui_sound:
        params["is_ui_sound"] = True
    if attenuation_max_distance > 0.0:
        params["attenuation_max_distance"] = attenuation_max_distance
    response = unreal.send_command("spawn_actor", params)
    return response.get("result", response)

@mcp.tool()
def spawn_blueprint_actor_in_level(
    blueprint_path: str,
    actor_name: str,
    location: List[float] = [0.0, 0.0, 0.0],
    rotation: List[float] = [0.0, 0.0, 0.0],
) -> Dict[str, Any]:
    """
    Spawn an existing Blueprint as an actor in the level.

    Use this to place a pre-made Blueprint (e.g., a Character Blueprint) into the world.

    Parameters:
    - blueprint_path: Content path to the Blueprint (e.g., "/Game/Characters/Robot/BP_RobotCharacter")
    - actor_name: Desired name for the spawned actor
    - location: [X, Y, Z] position in Unreal units
    - rotation: [Pitch, Yaw, Roll] in degrees
    """
    unreal = get_unreal_connection()
    response = unreal.send_command("spawn_blueprint_actor", {
        "blueprint_name": blueprint_path,
        "actor_name": actor_name,
        "location": location,
        "rotation": rotation,
    })
    return response.get("result", response)

@mcp.tool()
def spawn_physics_blueprint_actor (
    name: str,
    mesh_path: str = "/Engine/BasicShapes/Cube.Cube",
    location: List[float] = [0.0, 0.0, 0.0],
    mass: float = 1.0,
    simulate_physics: bool = True,
    gravity_enabled: bool = True,
    color: List[float] = None,  # Optional color parameter [R, G, B] or [R, G, B, A]
    scale: List[float] = [1.0, 1.0, 1.0]  # Default scale
) -> Dict[str, Any]:
    """
    Quickly spawn a single actor with physics, color, and a specific mesh.

    This is the primary function for creating simple objects with physics properties.
    It handles creating a temporary Blueprint, setting up the mesh, color, and physics,
    and then spawns the actor in the world. It's ideal for quickly adding
    dynamic objects to the scene without needing to manually create Blueprints.
    
    Args:
        color: Optional color as [R, G, B] or [R, G, B, A] where values are 0.0-1.0.
               If [R, G, B] is provided, alpha will be set to 1.0 automatically.
    """
    try:
        bp_name = f"{name}_BP"
        create_blueprint(bp_name, "Actor")
        add_component_to_blueprint(bp_name, "StaticMeshComponent", "Mesh", scale=scale)
        set_static_mesh_properties(bp_name, "Mesh", mesh_path)
        set_physics_properties(bp_name, "Mesh", simulate_physics, gravity_enabled, mass)

        # Set color if provided
        if color is not None:
            # Convert 3-value color [R,G,B] to 4-value [R,G,B,A] if needed
            if len(color) == 3:
                color = color + [1.0]  # Add alpha=1.0
            elif len(color) != 4:
                logger.warning(f"Invalid color format: {color}. Expected [R,G,B] or [R,G,B,A]. Skipping color.")
                color = None

            if color is not None:
                color_result = set_mesh_material_color(bp_name, "Mesh", color)
                if not color_result.get("success", False):
                    logger.warning(f"Failed to set color {color} for {bp_name}: {color_result.get('message', 'Unknown error')}")

        compile_blueprint(bp_name)
        result = spawn_blueprint_actor(bp_name, name, location)
        
        # Spawn the blueprint actor using helper function
        unreal = get_unreal_connection()
        result = spawn_blueprint_actor(unreal, bp_name, name, location)

        # Ensure proper scale is set on the spawned actor
        if result.get("success", False):
            spawned_name = result.get("result", {}).get("name", name)
            set_actor_transform(spawned_name, scale=scale)

        return result
    except Exception as e:
        logger.error(f"spawn_physics_blueprint_actor  error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def create_maze(
    rows: int = 8,
    cols: int = 8,
    cell_size: float = 300.0,
    wall_height: int = 3,
    location: List[float] = [0.0, 0.0, 0.0]
) -> Dict[str, Any]:
    """Create a proper solvable maze with entrance, exit, and guaranteed path using recursive backtracking algorithm."""
    try:
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
        import random
        spawned = []
        
        # Initialize maze grid - True means wall, False means open
        maze = [[True for _ in range(cols * 2 + 1)] for _ in range(rows * 2 + 1)]
        
        # Recursive backtracking maze generation
        def carve_path(row, col):
            # Mark current cell as path
            maze[row * 2 + 1][col * 2 + 1] = False
            
            # Random directions
            directions = [(0, 1), (1, 0), (0, -1), (-1, 0)]
            random.shuffle(directions)
            
            for dr, dc in directions:
                new_row, new_col = row + dr, col + dc
                
                # Check bounds
                if (0 <= new_row < rows and 0 <= new_col < cols and 
                    maze[new_row * 2 + 1][new_col * 2 + 1]):
                    
                    # Carve wall between current and new cell
                    maze[row * 2 + 1 + dr][col * 2 + 1 + dc] = False
                    carve_path(new_row, new_col)
        
        # Start carving from top-left corner
        carve_path(0, 0)
        
        # Create entrance and exit
        maze[1][0] = False  # Entrance on left side
        maze[rows * 2 - 1][cols * 2] = False  # Exit on right side
        
        # Build the actual maze in Unreal
        maze_height = rows * 2 + 1
        maze_width = cols * 2 + 1
        
        for r in range(maze_height):
            for c in range(maze_width):
                if maze[r][c]:  # If this is a wall
                    # Stack blocks to create wall height
                    for h in range(wall_height):
                        x_pos = location[0] + (c - maze_width/2) * cell_size
                        y_pos = location[1] + (r - maze_height/2) * cell_size
                        z_pos = location[2] + h * cell_size
                        
                        actor_name = f"Maze_Wall_{r}_{c}_{h}"
                        params = {
                            "name": actor_name,
                            "type": "StaticMeshActor",
                            "location": [x_pos, y_pos, z_pos],
                            "scale": [cell_size/100.0, cell_size/100.0, cell_size/100.0],
                            "static_mesh": "/Engine/BasicShapes/Cube.Cube"
                        }
                        resp = safe_spawn_actor(unreal, params)
                        if resp and resp.get("status") == "success":
                            spawned.append(resp)
        
        # Add entrance and exit markers
        entrance_marker = safe_spawn_actor(unreal, {
            "name": "Maze_Entrance",
            "type": "StaticMeshActor",
            "location": [location[0] - maze_width/2 * cell_size - cell_size, 
                       location[1] + (-maze_height/2 + 1) * cell_size, 
                       location[2] + cell_size],
            "scale": [0.5, 0.5, 0.5],
            "static_mesh": "/Engine/BasicShapes/Cylinder.Cylinder"
        })
        if entrance_marker and entrance_marker.get("status") == "success":
            spawned.append(entrance_marker)
            
        exit_marker = safe_spawn_actor(unreal, {
            "name": "Maze_Exit",
            "type": "StaticMeshActor", 
            "location": [location[0] + maze_width/2 * cell_size + cell_size,
                       location[1] + (-maze_height/2 + rows * 2 - 1) * cell_size,
                       location[2] + cell_size],
            "scale": [0.5, 0.5, 0.5],
            "static_mesh": "/Engine/BasicShapes/Sphere.Sphere"
        })
        if exit_marker and exit_marker.get("status") == "success":
            spawned.append(exit_marker)
        
        return {
            "success": True, 
            "actors": spawned, 
            "maze_size": f"{rows}x{cols}",
            "wall_count": len([block for block in spawned if "Wall" in block.get("name", "")]),
            "entrance": "Left side (cylinder marker)",
            "exit": "Right side (sphere marker)"
        }
    except Exception as e:
        logger.error(f"create_maze error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def get_available_materials(
    search_path: str = "/Game/",
    include_engine_materials: bool = True
) -> Dict[str, Any]:
    """Get a list of available materials in the project that can be applied to objects."""
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}
    
    try:
        params = {
            "search_path": search_path,
            "include_engine_materials": include_engine_materials
        }
        response = unreal.send_command("get_available_materials", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"get_available_materials error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def apply_material_to_actor(
    actor_name: str,
    material_path: str,
    material_slot: int = -1
) -> Dict[str, Any]:
    """Apply a material to an actor. Use material_slot=-1 (default) to apply to ALL slots."""
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}
    
    try:
        params = {
            "actor_name": actor_name,
            "material_path": material_path,
            "material_slot": material_slot
        }
        response = unreal.send_command("apply_material_to_actor", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"apply_material_to_actor error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def set_mesh_asset_material(
    mesh_path: str,
    material_path: str,
    material_slot: int = -1
) -> Dict[str, Any]:
    """
    Set the default material on a static mesh asset. This permanently changes
    the mesh's material so ALL instances use it. Use material_slot=-1 to apply to ALL slots.

    Parameters:
    - mesh_path: Content browser path to the static mesh (e.g., "/Game/Meshes/Rocks/SM_Boulder_01")
    - material_path: Content browser path to the material (e.g., "/Game/Materials/Rocks/M_Boulder_PBR")
    - material_slot: Which slot to set (-1 = all slots)
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "mesh_path": mesh_path,
            "material_path": material_path,
            "material_slot": material_slot
        }
        response = unreal.send_command("set_mesh_asset_material", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"set_mesh_asset_material error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def apply_material_to_blueprint(
    blueprint_name: str,
    component_name: str,
    material_path: str,
    material_slot: int = -1
) -> Dict[str, Any]:
    """Apply a material to a component in a Blueprint. Use material_slot=-1 (default) to apply to ALL slots."""
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}
    
    try:
        params = {
            "blueprint_name": blueprint_name,
            "component_name": component_name,
            "material_path": material_path,
            "material_slot": material_slot
        }
        response = unreal.send_command("apply_material_to_blueprint", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"apply_material_to_blueprint error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def get_actor_material_info(
    actor_name: str
) -> Dict[str, Any]:
    """Get information about the materials currently applied to an actor."""
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}
    
    try:
        params = {"actor_name": actor_name}
        response = unreal.send_command("get_actor_material_info", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"get_actor_material_info error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def set_mesh_material_color(
    blueprint_name: str,
    component_name: str,
    color: List[float],
    material_path: str = "/Engine/BasicShapes/BasicShapeMaterial",
    parameter_name: str = "BaseColor",
    material_slot: int = 0
) -> Dict[str, Any]:
    """Set material color on a mesh component using the proven color system."""
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}
    
    try:
        # Validate color format
        if not isinstance(color, list) or len(color) != 4:
            return {"success": False, "message": "Invalid color format. Must be a list of 4 float values [R, G, B, A]."}
        
        # Ensure all color values are floats between 0 and 1
        color = [float(min(1.0, max(0.0, val))) for val in color]
        
        # Set BaseColor parameter first
        params_base = {
            "blueprint_name": blueprint_name,
            "component_name": component_name,
            "color": color,
            "material_path": material_path,
            "parameter_name": "BaseColor",
            "material_slot": material_slot
        }
        response_base = unreal.send_command("set_mesh_material_color", params_base)
        
        # Set Color parameter second (for maximum compatibility)
        params_color = {
            "blueprint_name": blueprint_name,
            "component_name": component_name,
            "color": color,
            "material_path": material_path,
            "parameter_name": "Color",
            "material_slot": material_slot
        }
        response_color = unreal.send_command("set_mesh_material_color", params_color)
        
        # Return success if either parameter setting worked
        if (response_base and response_base.get("status") == "success") or (response_color and response_color.get("status") == "success"):
            return {
                "success": True, 
                "message": f"Color applied successfully to slot {material_slot}: {color}",
                "base_color_result": response_base,
                "color_result": response_color,
                "material_slot": material_slot
            }
        else:
            return {
                "success": False, 
                "message": f"Failed to set color parameters on slot {material_slot}. BaseColor: {response_base}, Color: {response_color}"
            }
            
    except Exception as e:
        logger.error(f"set_mesh_material_color error: {e}")
        return {"success": False, "message": str(e)}

# Advanced Town Generation System
@mcp.tool()
def create_town(
    town_size: str = "medium",  # "small", "medium", "large", "metropolis"
    building_density: float = 0.7,  # 0.0 to 1.0
    location: List[float] = [0.0, 0.0, 0.0],
    name_prefix: str = "Town",
    include_infrastructure: bool = True,
    architectural_style: str = "mixed"  # "modern", "cottage", "mansion", "mixed", "downtown", "futuristic"
) -> Dict[str, Any]:
    """Create a full dynamic town with buildings, streets, infrastructure, and vehicles."""
    try:
        import random
        random.seed()  # Use different seed each time for variety
        
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}
        
        logger.info(f"Creating {town_size} town with {building_density} density at {location}")
        
        # Define town parameters based on size
        town_params = {
            "small": {"blocks": 3, "block_size": 1500, "max_building_height": 5, "population": 20, "skyscraper_chance": 0.1},
            "medium": {"blocks": 5, "block_size": 2000, "max_building_height": 10, "population": 50, "skyscraper_chance": 0.3},
            "large": {"blocks": 7, "block_size": 2500, "max_building_height": 20, "population": 100, "skyscraper_chance": 0.5},
            "metropolis": {"blocks": 10, "block_size": 3000, "max_building_height": 40, "population": 200, "skyscraper_chance": 0.7}
        }
        
        params = town_params.get(town_size, town_params["medium"])
        blocks = params["blocks"]
        block_size = params["block_size"]
        max_height = params["max_building_height"]
        target_population = int(params["population"] * building_density)
        skyscraper_chance = params["skyscraper_chance"]
        
        all_spawned = []
        street_width = block_size * 0.3
        building_area = block_size * 0.7
        
        # Create street grid first
        logger.info("Creating street grid...")
        street_results = _create_street_grid(blocks, block_size, street_width, location, name_prefix)
        all_spawned.extend(street_results.get("actors", []))
        
        # Create buildings in each block
        logger.info("Placing buildings...")
        building_count = 0
        for block_x in range(blocks):
            for block_y in range(blocks):
                if building_count >= target_population:
                    break
                    
                # Skip some blocks randomly for variety
                if random.random() > building_density:
                    continue
                
                block_center_x = location[0] + (block_x - blocks/2) * block_size
                block_center_y = location[1] + (block_y - blocks/2) * block_size
                
                # Randomly choose building type based on style and location
                if architectural_style == "downtown" or architectural_style == "futuristic":
                    building_types = ["skyscraper", "office_tower", "apartment_complex", "shopping_mall", "parking_garage", "hotel"]
                elif architectural_style == "mixed":
                    # Central blocks get taller buildings
                    is_central = abs(block_x - blocks//2) <= 1 and abs(block_y - blocks//2) <= 1
                    if is_central and random.random() < skyscraper_chance:
                        building_types = ["skyscraper", "office_tower", "apartment_complex", "hotel", "shopping_mall"]
                    else:
                        building_types = ["house", "tower", "mansion", "commercial", "apartment_building", "restaurant", "store"]
                else:
                    building_types = [architectural_style] * 3 + ["commercial", "restaurant", "store"]
                
                building_type = random.choice(building_types)
                
                # Create building with variety
                building_result = _create_town_building(
                    building_type, 
                    [block_center_x, block_center_y, location[2]],
                    building_area,
                    max_height,
                    f"{name_prefix}_Building_{block_x}_{block_y}",
                    building_count
                )
                
                if building_result.get("status") == "success":
                    all_spawned.extend(building_result.get("actors", []))
                    building_count += 1
        
        # Add infrastructure if requested
        infrastructure_count = 0
        if include_infrastructure:
            logger.info("Adding infrastructure...")
            
            # Street lights
            light_results = _create_street_lights(blocks, block_size, location, name_prefix)
            all_spawned.extend(light_results.get("actors", []))
            infrastructure_count += len(light_results.get("actors", []))
            
            # Vehicles
            vehicle_results = _create_town_vehicles(blocks, block_size, street_width, location, name_prefix, target_population // 3)
            all_spawned.extend(vehicle_results.get("actors", []))
            infrastructure_count += len(vehicle_results.get("actors", []))
            
            # Parks and decorations
            decoration_results = _create_town_decorations(blocks, block_size, location, name_prefix)
            all_spawned.extend(decoration_results.get("actors", []))
            infrastructure_count += len(decoration_results.get("actors", []))
            
            
            # Add advanced infrastructure
            logger.info("Adding advanced infrastructure...")
            
            # Traffic lights at intersections
            traffic_results = _create_traffic_lights(blocks, block_size, location, name_prefix)
            all_spawned.extend(traffic_results.get("actors", []))
            infrastructure_count += len(traffic_results.get("actors", []))
            
            # Street signs and billboards
            signage_results = _create_street_signage(blocks, block_size, location, name_prefix, town_size)
            all_spawned.extend(signage_results.get("actors", []))
            infrastructure_count += len(signage_results.get("actors", []))
            
            # Sidewalks and crosswalks
            sidewalk_results = _create_sidewalks_crosswalks(blocks, block_size, street_width, location, name_prefix)
            all_spawned.extend(sidewalk_results.get("actors", []))
            infrastructure_count += len(sidewalk_results.get("actors", []))
            
            # Urban furniture (benches, trash cans, bus stops)
            furniture_results = _create_urban_furniture(blocks, block_size, location, name_prefix)
            all_spawned.extend(furniture_results.get("actors", []))
            infrastructure_count += len(furniture_results.get("actors", []))
            
            # Parking meters and hydrants
            utility_results = _create_street_utilities(blocks, block_size, location, name_prefix)
            all_spawned.extend(utility_results.get("actors", []))
            infrastructure_count += len(utility_results.get("actors", []))
            
            # Add plaza/square in center for large towns
            if town_size in ["large", "metropolis"]:
                plaza_results = _create_central_plaza(blocks, block_size, location, name_prefix)
                all_spawned.extend(plaza_results.get("actors", []))
                infrastructure_count += len(plaza_results.get("actors", []))
        
        return {
            "success": True,
            "town_stats": {
                "size": town_size,
                "density": building_density,
                "blocks": blocks,
                "buildings": building_count,
                "infrastructure_items": infrastructure_count,
                "total_actors": len(all_spawned),
                "architectural_style": architectural_style
            },
            "actors": all_spawned,
            "message": f"Created {town_size} town with {building_count} buildings and {infrastructure_count} infrastructure items"
        }
        
    except Exception as e:
        logger.error(f"create_town error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def create_castle_fortress(
    castle_size: str = "large",  # "small", "medium", "large", "epic"
    location: List[float] = [0.0, 0.0, 0.0],
    name_prefix: str = "Castle",
    include_siege_weapons: bool = True,
    include_village: bool = True,
    architectural_style: str = "medieval"  # "medieval", "fantasy", "gothic"
) -> Dict[str, Any]:
    """
    Create a massive castle fortress with walls, towers, courtyards, throne room,
    and surrounding village. Perfect for dramatic TikTok reveals showing
    the scale and detail of a complete medieval fortress.
    """
    try:
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}
        
        logger.info(f"Creating {castle_size} {architectural_style} castle fortress")
        all_actors = []
        
        # Get size parameters and calculate scaled dimensions
        params = get_castle_size_params(castle_size)
        dimensions = calculate_scaled_dimensions(params, scale_factor=2.0)
        
        # Build castle components using helper functions
        build_outer_bailey_walls(unreal, name_prefix, location, dimensions, all_actors)
        build_inner_bailey_walls(unreal, name_prefix, location, dimensions, all_actors)
        build_gate_complex(unreal, name_prefix, location, dimensions, all_actors)
        build_corner_towers(unreal, name_prefix, location, dimensions, architectural_style, all_actors)
        build_inner_corner_towers(unreal, name_prefix, location, dimensions, all_actors)
        build_intermediate_towers(unreal, name_prefix, location, dimensions, all_actors)
        build_central_keep(unreal, name_prefix, location, dimensions, all_actors)
        build_courtyard_complex(unreal, name_prefix, location, dimensions, all_actors)
        build_bailey_annexes(unreal, name_prefix, location, dimensions, all_actors)
        
        # Add optional components
        if include_siege_weapons:
            build_siege_weapons(unreal, name_prefix, location, dimensions, all_actors)
        
        if include_village:
            build_village_settlement(unreal, name_prefix, location, dimensions, castle_size, all_actors)
        
        # Add final touches
        build_drawbridge_and_moat(unreal, name_prefix, location, dimensions, all_actors)
        add_decorative_flags(unreal, name_prefix, location, dimensions, all_actors)
        
        logger.info(f"Castle fortress creation complete! Created {len(all_actors)} actors")

        
        return {
            "success": True,
            "message": f"Epic {castle_size} {architectural_style} castle fortress created with {len(all_actors)} elements!",
            "actors": all_actors,
            "stats": {
                "size": castle_size,
                "style": architectural_style,
                "wall_sections": int(dimensions["outer_width"]/200) * 2 + int(dimensions["outer_depth"]/200) * 2,
                "towers": dimensions["tower_count"],
                "has_village": include_village,
                "has_siege_weapons": include_siege_weapons,
                "total_actors": len(all_actors)
            }
        }
        
    except Exception as e:
        logger.error(f"create_castle_fortress error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def create_suspension_bridge(
    span_length: float = 6000.0,
    deck_width: float = 800.0,
    tower_height: float = 4000.0,
    cable_sag_ratio: float = 0.12,
    module_size: float = 200.0,
    location: List[float] = [0.0, 0.0, 0.0],
    orientation: str = "x",
    name_prefix: str = "Bridge",
    deck_mesh: str = "/Engine/BasicShapes/Cube.Cube",
    tower_mesh: str = "/Engine/BasicShapes/Cube.Cube",
    cable_mesh: str = "/Engine/BasicShapes/Cylinder.Cylinder",
    suspender_mesh: str = "/Engine/BasicShapes/Cylinder.Cylinder",
    dry_run: bool = False
) -> Dict[str, Any]:
    """
    Build a suspension bridge with towers, deck, cables, and suspenders.
    
    Creates a realistic suspension bridge with parabolic main cables, vertical
    suspenders, twin towers, and a multi-lane deck. Perfect for dramatic reveals
    showing engineering marvels.
    
    Args:
        span_length: Total span between towers
        deck_width: Width of the bridge deck
        tower_height: Height of support towers
        cable_sag_ratio: Sag as fraction of span (0.1-0.15 typical)
        module_size: Resolution for segments (affects actor count)
        location: Center point of the bridge
        orientation: "x" or "y" for bridge direction
        name_prefix: Prefix for all spawned actors
        deck_mesh: Mesh for deck segments
        tower_mesh: Mesh for tower components
        cable_mesh: Mesh for cable segments
        suspender_mesh: Mesh for vertical suspenders
        dry_run: If True, calculate metrics without spawning
    
    Returns:
        Dictionary with success status, spawned actors, and performance metrics
    """
    try:
        import time
        start_time = time.perf_counter()
        
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}
        
        logger.info(f"Creating suspension bridge: span={span_length}, width={deck_width}, height={tower_height}")
        
        all_actors = []
        
        # Calculate expected actor counts for dry run
        if dry_run:
            expected_towers = 10  # 2 towers with main, base, top, and 2 attachment points each
            expected_deck = max(1, int(span_length / module_size)) * max(1, int(deck_width / module_size))
            expected_cables = 2 * max(1, int(span_length / module_size))  # 2 main cables
            expected_suspenders = 2 * max(1, int(span_length / (module_size * 3)))  # Every 3 modules
            
            elapsed_ms = int((time.perf_counter() - start_time) * 1000)
            
            return {
                "success": True,
                "dry_run": True,
                "metrics": {
                    "total_actors": expected_towers + expected_deck + expected_cables + expected_suspenders,
                    "deck_segments": expected_deck,
                    "cable_segments": expected_cables,
                    "suspender_count": expected_suspenders,
                    "towers": expected_towers,
                    "span_length": span_length,
                    "deck_width": deck_width,
                    "est_area": span_length * deck_width,
                    "elapsed_ms": elapsed_ms
                }
            }
        
        # Build the bridge structure
        counts = build_suspension_bridge_structure(
            unreal,
            span_length,
            deck_width,
            tower_height,
            cable_sag_ratio,
            module_size,
            location,
            orientation,
            name_prefix,
            deck_mesh,
            tower_mesh,
            cable_mesh,
            suspender_mesh,
            all_actors
        )
        
        # Calculate metrics
        elapsed_ms = int((time.perf_counter() - start_time) * 1000)
        total_actors = sum(counts.values())
        
        logger.info(f"Bridge construction complete: {total_actors} actors in {elapsed_ms}ms")
        
        return {
            "success": True,
            "message": f"Created suspension bridge with {total_actors} components",
            "actors": all_actors,
            "metrics": {
                "total_actors": total_actors,
                "deck_segments": counts["deck_segments"],
                "cable_segments": counts["cable_segments"],
                "suspender_count": counts["suspenders"],
                "towers": counts["towers"],
                "span_length": span_length,
                "deck_width": deck_width,
                "est_area": span_length * deck_width,
                "elapsed_ms": elapsed_ms
            }
        }
        
    except Exception as e:
        logger.error(f"create_suspension_bridge error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def create_aqueduct(
    arches: int = 18,
    arch_radius: float = 600.0,
    pier_width: float = 200.0,
    tiers: int = 2,
    deck_width: float = 600.0,
    module_size: float = 200.0,
    location: List[float] = [0.0, 0.0, 0.0],
    orientation: str = "x",
    name_prefix: str = "Aqueduct",
    arch_mesh: str = "/Engine/BasicShapes/Cylinder.Cylinder",
    pier_mesh: str = "/Engine/BasicShapes/Cube.Cube",
    deck_mesh: str = "/Engine/BasicShapes/Cube.Cube",
    dry_run: bool = False
) -> Dict[str, Any]:
    """
    Build a multi-tier Roman-style aqueduct with arches and water channel.
    
    Creates a majestic aqueduct with repeating arches, support piers, and
    a water channel deck. Each tier has progressively smaller piers for
    realistic tapering. Perfect for showing ancient engineering.
    
    Args:
        arches: Number of arches per tier
        arch_radius: Radius of each arch
        pier_width: Width of support piers
        tiers: Number of vertical tiers (1-3 recommended)
        deck_width: Width of the water channel
        module_size: Resolution for segments (affects actor count)
        location: Starting point of the aqueduct
        orientation: "x" or "y" for aqueduct direction
        name_prefix: Prefix for all spawned actors
        arch_mesh: Mesh for arch segments (cylinder)
        pier_mesh: Mesh for support piers
        deck_mesh: Mesh for deck and walls
        dry_run: If True, calculate metrics without spawning
    
    Returns:
        Dictionary with success status, spawned actors, and performance metrics
    """
    try:
        import time
        start_time = time.perf_counter()
        
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}
        
        logger.info(f"Creating aqueduct: {arches} arches, {tiers} tiers, radius={arch_radius}")
        
        all_actors = []
        
        # Calculate dimensions
        total_length = arches * (2 * arch_radius + pier_width) + pier_width
        
        # Calculate expected actor counts for dry run
        if dry_run:
            # Arch segments per arch based on semicircle circumference
            arch_circumference = math.pi * arch_radius
            segments_per_arch = max(4, int(arch_circumference / module_size))
            expected_arch_segments = tiers * arches * segments_per_arch
            
            # Piers: (arches + 1) per tier
            expected_piers = tiers * (arches + 1)
            
            # Deck segments including side walls
            deck_length_segments = max(1, int(total_length / module_size))
            deck_width_segments = max(1, int(deck_width / module_size))
            expected_deck = deck_length_segments * deck_width_segments
            expected_deck += 2 * deck_length_segments  # Side walls
            
            elapsed_ms = int((time.perf_counter() - start_time) * 1000)
            
            return {
                "success": True,
                "dry_run": True,
                "metrics": {
                    "total_actors": expected_arch_segments + expected_piers + expected_deck,
                    "arch_segments": expected_arch_segments,
                    "pier_count": expected_piers,
                    "tiers": tiers,
                    "deck_segments": expected_deck,
                    "total_length": total_length,
                    "est_area": total_length * deck_width,
                    "elapsed_ms": elapsed_ms
                }
            }
        
        # Build the aqueduct structure
        counts = build_aqueduct_structure(
            unreal,
            arches,
            arch_radius,
            pier_width,
            tiers,
            deck_width,
            module_size,
            location,
            orientation,
            name_prefix,
            arch_mesh,
            pier_mesh,
            deck_mesh,
            all_actors
        )
        
        # Calculate metrics
        elapsed_ms = int((time.perf_counter() - start_time) * 1000)
        total_actors = sum(counts.values())
        
        logger.info(f"Aqueduct construction complete: {total_actors} actors in {elapsed_ms}ms")
        
        return {
            "success": True,
            "message": f"Created {tiers}-tier aqueduct with {arches} arches ({total_actors} components)",
            "actors": all_actors,
            "metrics": {
                "total_actors": total_actors,
                "arch_segments": counts["arch_segments"],
                "pier_count": counts["piers"],
                "tiers": tiers,
                "deck_segments": counts["deck_segments"],
                "total_length": total_length,
                "est_area": total_length * deck_width,
                "elapsed_ms": elapsed_ms
            }
        }
        
    except Exception as e:
        logger.error(f"create_aqueduct error: {e}")
        return {"success": False, "message": str(e)}



# ============================================================================
# Blueprint Node Graph Tool
# ============================================================================

@mcp.tool()
def add_node(
    blueprint_name: str,
    node_type: str,
    pos_x: float = 0,
    pos_y: float = 0,
    message: str = "",
    event_type: str = "BeginPlay",
    variable_name: str = "",
    target_function: str = "",
    target_blueprint: Optional[str] = None,
    target_class: Optional[str] = None,
    function_name: Optional[str] = None
) -> Dict[str, Any]:
    """
    Add a node to a Blueprint graph.

    Create various types of K2Nodes in a Blueprint's event graph or function graph.
    Supports 23 node types organized by category.

    Args:
        blueprint_name: Name of the Blueprint to modify
        node_type: Type of node to create. Supported types (23 total):

            CONTROL FLOW:
                "Branch" - Conditional execution (if/then/else)
                "Comparison" - Arithmetic/logical operators (==, !=, <, >, AND, OR, etc.)
                    ℹ️ Types can be changed via set_node_property with action="set_pin_type"
                "Switch" - Switch on byte/enum value with cases
                    ℹ️ Creates 1 pin at creation; add more via set_node_property with action="add_pin"
                "SwitchEnum" - Switch on enum type (auto-generates pins per enum value)
                    ℹ️ Creates pins based on enum; change enum via set_node_property with action="set_enum_type"
                "SwitchInteger" - Switch on integer value with cases
                    ℹ️ Creates 1 pin at creation; add more via set_node_property with action="add_pin"
                "ExecutionSequence" - Sequential execution with multiple outputs
                    ℹ️ Creates 1 pin at creation; add/remove via set_node_property (add_pin/remove_pin)

            DATA:
                "VariableGet" - Read a variable value (⚠️ variable must exist in Blueprint)
                "VariableSet" - Set a variable value (⚠️ variable must exist and be assignable)
                "MakeArray" - Create array from individual inputs
                    ℹ️ Creates 1 pin at creation; add/remove via set_node_property with action="set_num_elements"

            CASTING:
                "DynamicCast" - Cast object to specific class (⚠️ handle "Cast Failed" output)
                "ClassDynamicCast" - Cast class reference to derived class (⚠️ handle failure cases)
                "CastByteToEnum" - Convert byte value to enum (⚠️ byte must be valid enum range)

            UTILITY:
                "Print" - Debug output to screen/log (configurable duration and color)
                "CallFunction" - Call any blueprint/engine function (⚠️ function must exist)
                "Select" - Choose between two inputs based on boolean condition
                "SpawnActor" - Spawn actor from class (⚠️ class must derive from Actor)

            SPECIALIZED:
                "Timeline" - Animation timeline playback with curve tracks
                    ⚠️ REQUIRES MANUAL IMPLEMENTATION: Animation curves must be added in editor
                "GetDataTableRow" - Query row from data table (⚠️ DataTable must exist)
                "AddComponentByClass" - Dynamically add component to actor
                "Self" - Reference to current actor/object
                "Knot" - Invisible reroute node (wire organization only)

            EVENT:
                "Event" - Blueprint event (specify event_type: BeginPlay, Tick, etc.)
                    ℹ️ Tick events run every frame - be mindful of performance impact

        pos_x: X position in graph (default: 0)
        pos_y: Y position in graph (default: 0)
        message: For Print nodes, the text to print
        event_type: For Event nodes, the event name (BeginPlay, Tick, Destroyed, etc.)
        variable_name: For Variable nodes, the variable name
        target_function: For CallFunction nodes, the function to call
        target_blueprint: For CallFunction nodes, optional path to target Blueprint
        target_class: For CallFunction nodes, UClass name to search for the function (e.g. "Pawn", "Character", "Actor", "CharacterMovementComponent"). Without this, only searches UKismetSystemLibrary.
        function_name: Optional name of function graph to add node to (if None, uses EventGraph)

    Returns:
        Dictionary with success status, node_id, and position

    Important Notes:
        - Most nodes can have pins modified after creation via set_node_property
        - Dynamic pin management: Switch/SwitchEnum/ExecutionSequence/MakeArray support pin operations
        - Timeline is the ONLY node requiring manual implementation (curves must be added in editor)
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        node_params = {
            "pos_x": pos_x,
            "pos_y": pos_y
        }

        if message:
            node_params["message"] = message
        if event_type:
            node_params["event_type"] = event_type
        if variable_name:
            node_params["variable_name"] = variable_name
        if target_function:
            node_params["target_function"] = target_function
        if target_blueprint:
            node_params["target_blueprint"] = target_blueprint
        if target_class:
            node_params["target_class"] = target_class
        if function_name:
            node_params["function_name"] = function_name

        result = node_manager.add_node(
            unreal,
            blueprint_name,
            node_type,
            node_params
        )

        return result

    except Exception as e:
        logger.error(f"add_node error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def connect_nodes(
    blueprint_name: str,
    source_node_id: str,
    source_pin_name: str,
    target_node_id: str,
    target_pin_name: str,
    function_name: Optional[str] = None
) -> Dict[str, Any]:
    """
    Connect two nodes in a Blueprint graph.

    Links a source pin to a target pin between existing nodes in a Blueprint's event graph or function graph.

    Args:
        blueprint_name: Name of the Blueprint to modify
        source_node_id: ID of the source node
        source_pin_name: Name of the output pin on the source node
        target_node_id: ID of the target node
        target_pin_name: Name of the input pin on the target node
        function_name: Optional name of function graph (if None, uses EventGraph)

    Returns:
        Dictionary with success status and connection details
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        result = connector_manager.connect_nodes(
            unreal,
            blueprint_name,
            source_node_id,
            source_pin_name,
            target_node_id,
            target_pin_name,
            function_name
        )

        return result
    except Exception as e:
        logger.error(f"connect_nodes error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def create_variable(
    blueprint_name: str,
    variable_name: str,
    variable_type: str,
    default_value: Any = None,
    is_public: bool = False,
    tooltip: str = "",
    category: str = "Default"
) -> Dict[str, Any]:
    """
    Create a variable in a Blueprint.

    Adds a new variable to a Blueprint with specified type, default value, and properties.

    Args:
        blueprint_name: Name of the Blueprint to modify
        variable_name: Name of the variable to create
        variable_type: Type of the variable ("bool", "int", "float", "string", "vector", "rotator")
        default_value: Default value for the variable (optional)
        is_public: Whether the variable should be public/editable (default: False)
        tooltip: Tooltip text for the variable (optional)
        category: Category for organizing variables (default: "Default")

    Returns:
        Dictionary with success status and variable details
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        result = variable_manager.create_variable(
            unreal,
            blueprint_name,
            variable_name,
            variable_type,
            default_value,
            is_public,
            tooltip,
            category
        )

        return result
    except Exception as e:
        logger.error(f"create_variable error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def set_blueprint_variable_properties(
    blueprint_name: str,
    variable_name: str,
    var_name: Optional[str] = None,
    var_type: Optional[str] = None,
    is_blueprint_readable: Optional[bool] = None,
    is_blueprint_writable: Optional[bool] = None,
    is_public: Optional[bool] = None,
    is_editable_in_instance: Optional[bool] = None,
    tooltip: Optional[str] = None,
    category: Optional[str] = None,
    default_value: Any = None,
    expose_on_spawn: Optional[bool] = None,
    expose_to_cinematics: Optional[bool] = None,
    slider_range_min: Optional[str] = None,
    slider_range_max: Optional[str] = None,
    value_range_min: Optional[str] = None,
    value_range_max: Optional[str] = None,
    units: Optional[str] = None,
    bitmask: Optional[bool] = None,
    bitmask_enum: Optional[str] = None,
    replication_enabled: Optional[bool] = None,
    replication_condition: Optional[int] = None,
    is_private: Optional[bool] = None
) -> Dict[str, Any]:
    """
    Modify properties of an existing Blueprint variable without deleting it.

    Preserves all VariableGet and VariableSet nodes connected to this variable.

    Args:
        blueprint_name: Name of the Blueprint to modify
        variable_name: Name of the variable to modify

        var_name: Rename the variable (optional)
            ✅ PASS - VarDesc->VarName works correctly

        var_type: Change variable type (optional)
            ✅ PASS - VarDesc->VarType works correctly (int→float returns "real")

        is_blueprint_readable: Allow reading in Blueprint (VariableGet) (optional)
            ✅ PASS - CPF_BlueprintReadOnly flag (inverted logic)

        is_blueprint_writable: Allow writing in Blueprint (Set) (optional)
            ✅ PASS - CPF_BlueprintReadOnly flag (inverted logic)
            ⚠️ NOT returned by get_variable_details()

        is_public: Visible in Blueprint editor (optional)
            ✅ PASS - Controls variable visibility

        is_editable_in_instance: Modifiable on instances (optional)
            ✅ PASS - CPF_DisableEditOnInstance flag (inverted logic)

        tooltip: Variable description (optional)
            ✅ PASS - Metadata MD_Tooltip works correctly

        category: Variable category (optional)
            ✅ PASS - Direct property Category works

        default_value: New default value (optional)
            ✅ PASS - Works but get_variable_details() returns empty string

        expose_on_spawn: Show in spawn dialog (optional)
            ✅ PASS - Metadata MD_ExposeOnSpawn works
            ⚠️ Requires is_editable_in_instance=true to be visible
            ⚠️ NOT returned by get_variable_details()

        expose_to_cinematics: Expose to cinematics (optional)
            ✅ PASS - CPF_Interp flag works correctly
            ⚠️ NOT returned by get_variable_details()

        slider_range_min: UI slider minimum value (optional)
            ✅ PASS - Metadata MD_UIMin works (string value)
            ⚠️ NOT returned by get_variable_details()

        slider_range_max: UI slider maximum value (optional)
            ✅ PASS - Metadata MD_UIMax works (string value)
            ⚠️ NOT returned by get_variable_details()

        value_range_min: Clamp minimum value (optional)
            ✅ PASS - Metadata MD_ClampMin works (string value)
            ⚠️ NOT returned by get_variable_details()

        value_range_max: Clamp maximum value (optional)
            ✅ PASS - Metadata MD_ClampMax works (string value)
            ⚠️ NOT returned by get_variable_details()

        units: Display units (optional)
            ⚠️ PARTIAL - Metadata MD_Units works for value display (e.g., "0.0 cm")
            ❌ UI dropdown stays at "None" (Unreal Editor limitation - dropdown doesn't sync with metadata)
            ⚠️ Use long format: "Centimeters", "Meters" (not "cm", "m")
            ⚠️ NOT returned by get_variable_details()

        bitmask: Treat as bitmask (optional)
            ✅ PASS - Metadata TEXT("Bitmask") works correctly
            ⚠️ NOT returned by get_variable_details()

        bitmask_enum: Bitmask enum type (optional)
            ✅ PASS - Metadata TEXT("BitmaskEnum") works
            ⚠️ REQUIRES full path format: "/Script/ModuleName.EnumName"
            ❌ Short names generate warning and don't sync dropdown
            ✅ Validated enums (use FULL PATHS):
                - /Script/UniversalObjectLocator.ELocatorResolveFlags
                - /Script/JsonObjectGraph.EJsonStringifyFlags
                - /Script/MediaAssets.EMediaAudioCaptureDeviceFilter
                - /Script/MediaAssets.EMediaVideoCaptureDeviceFilter
                - /Script/MediaAssets.EMediaWebcamCaptureDeviceFilter
                - /Script/Engine.EAnimAssetCurveFlags
                - /Script/Engine.EHardwareDeviceSupportedFeatures
                - /Script/EnhancedInput.EMappingQueryIssue
                - /Script/EnhancedInput.ETriggerEvent
            ⚠️ NOT returned by get_variable_details()

        replication_enabled: Enable network replication (CPF_Net flag) (optional)
            ✅ PASS - CPF_Net flag works - Changes "Replication" dropdown (None ↔ Replicated)
            ⚠️ NOT returned by get_variable_details()

        replication_condition: Network replication condition (ELifetimeCondition 0-7) (optional)
            ✅ PASS - VarDesc->ReplicationCondition works
            ✅ Changes "Replication Condition" dropdown (e.g., None → Initial Only)
            ⚠️ Values: 0=None, 1=InitialOnly, 2=OwnerOnly, 3=SkipOwner, 4=SimulatedOnly, 5=AutonomousOnly, 6=SimulatedOrPhysics, 7=InitialOrOwner
            ✅ Returned by get_variable_details() as "replication"

        is_private: Set variable as private (optional)
            ❌ UNRESOLVED - Property flag/metadata not yet identified
            ⚠️ Attempted CPF_NativeAccessSpecifierPrivate flag and MD_AllowPrivateAccess metadata - neither work
            ⚠️ The property that controls "Privé" (Private) checkbox remains unknown
            ⚠️ Parameter exists but has no effect on UI - do NOT use until resolved

    Returns:
        Dictionary with success status and updated properties
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        result = variable_manager.set_blueprint_variable_properties(
            unreal,
            blueprint_name,
            variable_name,
            var_name,
            var_type,
            is_blueprint_readable,
            is_blueprint_writable,
            is_public,
            is_editable_in_instance,
            tooltip,
            category,
            default_value,
            expose_on_spawn,
            expose_to_cinematics,
            slider_range_min,
            slider_range_max,
            value_range_min,
            value_range_max,
            units,
            bitmask,
            bitmask_enum,
            replication_enabled,
            replication_condition,
            is_private
        )

        return result
    except Exception as e:
        logger.error(f"set_blueprint_variable_properties error: {e}")
        return {"success": False, "message": str(e)}

@mcp.tool()
def add_event_node(
    blueprint_name: str,
    event_name: str,
    pos_x: float = 0,
    pos_y: float = 0
) -> Dict[str, Any]:
    """
    Add an event node to a Blueprint graph.

    Create specialized event nodes (ReceiveBeginPlay, ReceiveTick, etc.)
    in a Blueprint's event graph at specified positions.

    Args:
        blueprint_name: Name of the Blueprint to modify
        event_name: Name of the event (e.g., "ReceiveBeginPlay", "ReceiveTick", "ReceiveDestroyed")
        pos_x: X position in graph (default: 0)
        pos_y: Y position in graph (default: 0)

    Returns:
        Dictionary with success status, node_id, event_name, and position
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        result = event_manager.add_event_node(
            unreal,
            blueprint_name,
            event_name,
            pos_x,
            pos_y
        )

        return result
    except Exception as e:
        logger.error(f"add_event_node error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def delete_node(
    blueprint_name: str,
    node_id: str,
    function_name: Optional[str] = None
) -> Dict[str, Any]:
    """
    Delete a node from a Blueprint graph.

    Removes a node and all its connections from either the EventGraph
    or a specific function graph.

    Args:
        blueprint_name: Name of the Blueprint to modify
        node_id: ID of the node to delete (NodeGuid or node name)
        function_name: Name of function graph (optional, defaults to EventGraph)

    Returns:
        Dictionary with success status and deleted_node_id
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        result = node_deleter.delete_node(
            unreal,
            blueprint_name,
            node_id,
            function_name
        )
        return result
    except Exception as e:
        logger.error(f"delete_node error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def set_node_property(
    blueprint_name: str,
    node_id: str,
    property_name: str = "",
    property_value: Any = None,
    function_name: Optional[str] = None,
    action: Optional[str] = None,
    pin_type: Optional[str] = None,
    pin_name: Optional[str] = None,
    enum_type: Optional[str] = None,
    new_type: Optional[str] = None,
    target_type: Optional[str] = None,
    target_function: Optional[str] = None,
    target_class: Optional[str] = None,
    event_type: Optional[str] = None,
    default_value: Any = None
) -> Dict[str, Any]:
    """
    Set a property on a Blueprint node or perform semantic node editing.

    This function supports both simple property modifications and advanced semantic
    node editing operations (pin management, type modifications, reference updates).

    Args:
        blueprint_name: Name of the Blueprint to modify
        node_id: ID of the node to modify
        property_name: Name of property to set (legacy mode, used if action not specified)
        property_value: Value to set (legacy mode)
        function_name: Name of function graph (optional, defaults to EventGraph)
        action: Semantic action to perform - can be one of:
            Phase 1 (Pin Management):
                - "add_pin": Add a pin to a node (requires pin_type)
                - "remove_pin": Remove a pin from a node (requires pin_name)
                - "set_enum_type": Set enum type on a node (requires enum_type)
            Phase 2 (Type Modification):
                - "set_pin_type": Change pin type on comparison nodes (requires pin_name, new_type)
                - "set_value_type": Change value type on select nodes (requires new_type)
                - "set_cast_target": Change cast target type (requires target_type)
            Phase 3 (Reference Updates - DESTRUCTIVE):
                - "set_function_call": Change function being called (requires target_function)
                - "set_event_type": Change event type (requires event_type)
            Phase 4 (Pin Defaults):
                - "set_pin_default": Set default value on a pin (requires pin_name, default_value)
                    For primitive pins (float, int, bool, string): pass the value as string
                    For object pins (UObject references): pass the asset path (e.g., "/Game/Animations/MyAnim.MyAnim")

    Semantic action parameters:
        pin_type: Type of pin to add ("SwitchCase", "ExecutionOutput", "ArrayElement", "EnumValue")
        pin_name: Name of pin to remove or modify, or pin to set default on
        enum_type: Full path to enum type (e.g., "/Game/Enums/ECardinalDirection")
        new_type: New type for pin or value ("int", "float", "string", "bool", "vector", etc.)
        target_type: Target class path for casting
        target_function: Name of function to call
        target_class: Optional class containing the function
        event_type: Event type (e.g., "BeginPlay", "Tick", "Destroyed")
        default_value: Default value for a pin (used with action="set_pin_default")

    Returns:
        Dictionary with success status and details

    Supported legacy properties by node type:
        - Print nodes: "message", "duration", "text_color"
        - Variable nodes: "variable_name"
        - All nodes: "pos_x", "pos_y", "comment"

    Examples:
        Legacy mode (set simple property):
            set_node_property(
                blueprint_name="MyActorBlueprint",
                node_id="K2Node_1234567890",
                property_name="message",
                property_value="Hello World!"
            )

        Semantic mode (add pin):
            set_node_property(
                blueprint_name="MyActorBlueprint",
                node_id="K2Node_Switch_123",
                action="add_pin",
                pin_type="SwitchCase"
            )

        Semantic mode (set enum type):
            set_node_property(
                blueprint_name="MyActorBlueprint",
                node_id="K2Node_SwitchEnum_456",
                action="set_enum_type",
                enum_type="ECardinalDirection"
            )

        Semantic mode (change function call):
            set_node_property(
                blueprint_name="MyActorBlueprint",
                node_id="K2Node_CallFunction_789",
                action="set_function_call",
                target_function="BeginPlay",
                target_class="APawn"
            )

        Semantic mode (set pin default - primitive):
            set_node_property(
                blueprint_name="MyActorBlueprint",
                node_id="K2Node_CallFunction_123",
                action="set_pin_default",
                pin_name="NewSpeed",
                default_value="800.0"
            )

        Semantic mode (set pin default - object/asset):
            set_node_property(
                blueprint_name="MyActorBlueprint",
                node_id="K2Node_CallFunction_456",
                action="set_pin_default",
                pin_name="AnimSequence",
                default_value="/Game/Animations/MyAnim.MyAnim"
            )
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        # Build kwargs for semantic actions
        kwargs = {}
        if action is not None:
            if pin_type is not None:
                kwargs["pin_type"] = pin_type
            if pin_name is not None:
                kwargs["pin_name"] = pin_name
            if enum_type is not None:
                kwargs["enum_type"] = enum_type
            if new_type is not None:
                kwargs["new_type"] = new_type
            if target_type is not None:
                kwargs["target_type"] = target_type
            if target_function is not None:
                kwargs["target_function"] = target_function
            if target_class is not None:
                kwargs["target_class"] = target_class
            if event_type is not None:
                kwargs["event_type"] = event_type
            if default_value is not None:
                kwargs["default_value"] = str(default_value)

        result = node_properties.set_node_property(
            unreal,
            blueprint_name,
            node_id,
            property_name,
            property_value,
            function_name,
            action,
            **kwargs
        )
        return result
    except Exception as e:
        logger.error(f"set_node_property error: {e}", exc_info=True)
        return {"success": False, "message": str(e)}


@mcp.tool()
def create_function(
    blueprint_name: str,
    function_name: str,
    return_type: str = "void"
) -> Dict[str, Any]:
    """
    Create a new function in a Blueprint.

    Args:
        blueprint_name: Name of the Blueprint to modify
        function_name: Name for the new function
        return_type: Return type of the function (default: "void")

    Returns:
        Dictionary with function_name, graph_id or error
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        result = function_manager.create_function_handler(
            unreal,
            blueprint_name,
            function_name,
            return_type
        )
        return result
    except Exception as e:
        logger.error(f"create_function error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def add_function_input(
    blueprint_name: str,
    function_name: str,
    param_name: str,
    param_type: str,
    is_array: bool = False
) -> Dict[str, Any]:
    """
    Add an input parameter to a Blueprint function.

    Args:
        blueprint_name: Name of the Blueprint to modify
        function_name: Name of the function
        param_name: Name of the input parameter
        param_type: Type of the parameter (bool, int, float, string, vector, etc.)
        is_array: Whether the parameter is an array (default: False)

    Returns:
        Dictionary with param_name, param_type, and direction or error
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        result = function_io.add_function_input_handler(
            unreal,
            blueprint_name,
            function_name,
            param_name,
            param_type,
            is_array
        )
        return result
    except Exception as e:
        logger.error(f"add_function_input error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def add_function_output(
    blueprint_name: str,
    function_name: str,
    param_name: str,
    param_type: str,
    is_array: bool = False
) -> Dict[str, Any]:
    """
    Add an output parameter to a Blueprint function.

    Args:
        blueprint_name: Name of the Blueprint to modify
        function_name: Name of the function
        param_name: Name of the output parameter
        param_type: Type of the parameter (bool, int, float, string, vector, etc.)
        is_array: Whether the parameter is an array (default: False)

    Returns:
        Dictionary with param_name, param_type, and direction or error
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        result = function_io.add_function_output_handler(
            unreal,
            blueprint_name,
            function_name,
            param_name,
            param_type,
            is_array
        )
        return result
    except Exception as e:
        logger.error(f"add_function_output error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def delete_function(
    blueprint_name: str,
    function_name: str
) -> Dict[str, Any]:
    """
    Delete a function from a Blueprint.

    Args:
        blueprint_name: Name of the Blueprint to modify
        function_name: Name of the function to delete

    Returns:
        Dictionary with deleted_function_name or error
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        result = function_manager.delete_function_handler(
            unreal,
            blueprint_name,
            function_name
        )
        return result
    except Exception as e:
        logger.error(f"delete_function error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def rename_function(
    blueprint_name: str,
    old_function_name: str,
    new_function_name: str
) -> Dict[str, Any]:
    """
    Rename a function in a Blueprint.

    Args:
        blueprint_name: Name of the Blueprint to modify
        old_function_name: Current name of the function
        new_function_name: New name for the function

    Returns:
        Dictionary with new_function_name or error
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        result = function_manager.rename_function_handler(
            unreal,
            blueprint_name,
            old_function_name,
            new_function_name
        )
        return result
    except Exception as e:
        logger.error(f"rename_function error: {e}")
        return {"success": False, "message": str(e)}


# ============================================================================
# Material Graph Expression Tools
# ============================================================================

@mcp.tool()
def create_material_asset(
    name: str,
    path: str = "/Game/Materials/"
) -> Dict[str, Any]:
    """
    Create a new empty material asset for building a custom material graph.

    Use this when you need to create a material with custom expression nodes,
    not just basic PBR properties. After creating, use add_material_expression
    to add nodes and connect_material_expressions to wire them up.

    Parameters:
    - name: The name of the material (e.g., "M_GroundBlend")
    - path: The content browser path (default: "/Game/Materials/")

    Returns:
        Dictionary with material path and success status.
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        response = unreal.send_command("create_material_asset", {
            "name": name,
            "path": path
        })
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"create_material_asset error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def add_material_expression(
    material_path: str,
    expression_type: str,
    pos_x: float = 0,
    pos_y: float = 0,
    expression_params: Dict[str, Any] = None
) -> Dict[str, Any]:
    """
    Add a material expression node to a material graph.

    Parameters:
    - material_path: Path to the material (e.g., "/Game/Materials/M_GroundBlend")
    - expression_type: Type of expression to create. Options:
        CONSTANTS: "Constant", "Constant2Vector", "Constant3Vector", "Constant4Vector"
        PARAMETERS: "ScalarParameter", "VectorParameter", "TextureSampleParameter2D"
        MATH: "Add", "Subtract", "Multiply", "Divide", "Power", "Abs", "Clamp", "OneMinus", "Lerp", "DotProduct", "Saturate"
        TEXTURE: "TextureSample", "TextureCoordinate", "Panner", "Rotator"
        LANDSCAPE: "LandscapeLayerBlend", "LandscapeLayerCoords"
        NORMALS: "VertexNormalWS", "PixelNormalWS"
        UTILITY: "WorldPosition", "ObjectPosition", "VertexColor", "Time", "ComponentMask", "Sine", "Cosine", "AppendVector"
    - pos_x: X position in material editor
    - pos_y: Y position in material editor
    - expression_params: Expression-specific parameters, e.g.:
        - For Constant: {"value": 0.5}
        - For Constant3Vector: {"r": 1.0, "g": 0.5, "b": 0.2} or {"color": [1.0, 0.5, 0.2]}
        - For ScalarParameter: {"parameter_name": "Roughness", "default_value": 0.5}
        - For TextureSample: {"texture_path": "/Game/Textures/T_Ground"}
        - For TextureCoordinate: {"u_tiling": 0.1, "v_tiling": 0.1}
        - For ComponentMask: {"r": true, "g": true, "b": false, "a": false}
        - For Math nodes: {"const_a": 0.5, "const_b": 1.0}
        - For LandscapeLayerCoords: {"mapping_scale": 5.0, "mapping_rotation": 0.0, "mapping_type": "XY"}
        - For Power: {"const_exponent": 2.0}

    Returns:
        Dictionary with expression_id (for use in connections) and success status.

    Example:
        # Add a WorldPosition node
        add_material_expression("/Game/Materials/M_Ground", "WorldPosition", -600, 0)

        # Add a TextureSample with a texture
        add_material_expression("/Game/Materials/M_Ground", "TextureSample", -400, 0,
                               {"texture_path": "/Game/Textures/T_Ground"})
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "material_path": material_path,
            "expression_type": expression_type,
            "pos_x": pos_x,
            "pos_y": pos_y
        }
        if expression_params:
            params["expression_params"] = expression_params

        response = unreal.send_command("add_material_expression", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"add_material_expression error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def connect_material_expressions(
    material_path: str,
    source_expression_id: str,
    target_expression_id: str,
    input_name: str,
    output_index: int = 0
) -> Dict[str, Any]:
    """
    Connect two material expressions together.

    Parameters:
    - material_path: Path to the material
    - source_expression_id: ID of the source expression (from add_material_expression)
    - target_expression_id: ID of the target expression
    - input_name: Name of the input on the target expression:
        - For Lerp: "a", "b", "alpha"
        - For Math (Add/Multiply/etc): "a", "b"
        - For DotProduct: "a", "b"
        - For Power: "base", "exponent"
        - For TextureSample: "coordinates" or "uv"
        - For Sine/Cosine/OneMinus/Abs/Saturate: "input"
        - For ComponentMask: "input"
        - For Clamp: "input", "min", "max"
    - output_index: Output index of the source expression (default 0)

    Returns:
        Dictionary with connection status.

    Example:
        # Connect WorldPosition to ComponentMask input
        connect_material_expressions("/Game/M_Ground", "expr_worldpos", "expr_mask", "input")

        # Connect two textures to a Lerp
        connect_material_expressions("/Game/M_Ground", "tex1_id", "lerp_id", "a")
        connect_material_expressions("/Game/M_Ground", "tex2_id", "lerp_id", "b")
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        response = unreal.send_command("connect_material_expressions", {
            "material_path": material_path,
            "source_expression_id": source_expression_id,
            "target_expression_id": target_expression_id,
            "input_name": input_name,
            "output_index": output_index
        })
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"connect_material_expressions error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def connect_to_material_output(
    material_path: str,
    expression_id: str,
    material_property: str,
    output_index: int = 0
) -> Dict[str, Any]:
    """
    Connect a material expression to a material output property.

    Parameters:
    - material_path: Path to the material
    - expression_id: ID of the expression to connect
    - material_property: The material property to connect to:
        "BaseColor", "Metallic", "Specular", "Roughness", "Anisotropy",
        "EmissiveColor", "Opacity", "OpacityMask", "Normal", "Tangent",
        "WorldPositionOffset", "SubsurfaceColor", "AmbientOcclusion"
    - output_index: Output index of the expression (default 0)

    Returns:
        Dictionary with connection status.

    Example:
        # Connect final Lerp to BaseColor
        connect_to_material_output("/Game/M_Ground", "lerp_final", "BaseColor")

        # Connect scalar parameter to Roughness
        connect_to_material_output("/Game/M_Ground", "roughness_param", "Roughness")
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        response = unreal.send_command("connect_to_material_output", {
            "material_path": material_path,
            "expression_id": expression_id,
            "material_property": material_property,
            "output_index": output_index
        })
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"connect_to_material_output error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def get_material_graph(
    material_path: str
) -> Dict[str, Any]:
    """
    Get the full graph structure of a material including all expressions.

    Parameters:
    - material_path: Path to the material to inspect

    Returns:
        Dictionary with:
        - expressions: List of all expressions with id, type, position
        - expression_count: Total number of expressions

    Example:
        get_material_graph("/Game/Materials/M_EarthTone")
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        response = unreal.send_command("get_material_graph", {
            "material_path": material_path
        })
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"get_material_graph error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def set_material_expression_property(
    material_path: str,
    expression_id: str,
    extra_properties: str = "",
    properties: dict = None,
) -> Dict[str, Any]:
    """
    Set properties on an existing material expression.

    Parameters:
    - material_path: Path to the material
    - expression_id: ID of the expression to modify
    - properties: Dict of property-value pairs to set (varies by expression type)
        - For Constant: {"value": 0.5}
        - For Constant3Vector: {"r": 1.0, "g": 0.5, "b": 0.2} or {"color": [1.0, 0.5, 0.2]}
        - For ScalarParameter: {"parameter_name": "Roughness", "default_value": 0.5}
        - For TextureCoordinate: {"u_tiling": 0.1, "v_tiling": 0.1}
        - For ComponentMask: {"r": true, "g": false, "b": false, "a": false}
        - For TextureSample: {"texture_path": "/Game/T_Tex", "sampler_type": "Normal"}
          sampler_type options: "Color", "Normal", "Masks", "LinearColor", "Grayscale"

    Returns:
        Dictionary with success status.

    Example:
        set_material_expression_property("/Game/M_Ground", "texcoord1",
                                         {"u_tiling": 0.08, "v_tiling": 0.08})
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "material_path": material_path,
            "expression_id": expression_id,
        }
        # Support properties as dict (from MCP schema) or as **kwargs (direct Python calls)
        if properties:
            if isinstance(properties, str):
                import json as _json
                properties = _json.loads(properties)
            params.update(properties)
        # extra_properties is a legacy string param, ignore it
        response = unreal.send_command("set_material_expression_property", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"set_material_expression_property error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def delete_material_expression(
    material_path: str,
    expression_id: str
) -> Dict[str, Any]:
    """
    Delete a material expression from a material.

    Parameters:
    - material_path: Path to the material
    - expression_id: ID of the expression to delete

    Returns:
        Dictionary with deleted_expression_id and success status.
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        response = unreal.send_command("delete_material_expression", {
            "material_path": material_path,
            "expression_id": expression_id
        })
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"delete_material_expression error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def recompile_material(
    material_path: str
) -> Dict[str, Any]:
    """
    Force recompilation of a material after making changes.

    Parameters:
    - material_path: Path to the material to recompile

    Returns:
        Dictionary with recompilation status.
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        response = unreal.send_command("recompile_material", {
            "material_path": material_path
        })
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"recompile_material error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def configure_landscape_layer_blend(
    material_path: str,
    expression_id: str,
    layers: list
) -> Dict[str, Any]:
    """
    Configure layers in a LandscapeLayerBlend material expression.

    This is essential for setting up landscape painting. The layer names defined here
    must match the layer info objects you add to the landscape.

    Parameters:
    - material_path: Path to the material (e.g., "/Game/Materials/M_Landscape")
    - expression_id: ID of the LandscapeLayerBlend expression (use get_material_graph to find it)
    - layers: List of layer configurations, each with:
        - name: Layer name (must match layer info name, e.g., "Grass", "Dirt", "Rock")
        - blend_type: "LB_WeightBlend" (default), "LB_AlphaBlend", or "LB_HeightBlend"
        - preview_weight: Optional float for preview (default 0.0)

    Returns:
        Dictionary with success status and layer count.

    Example:
        configure_landscape_layer_blend("/Game/Materials/M_Landscape",
            "MaterialExpressionLandscapeLayerBlend_0",
            [{"name": "Grass", "blend_type": "LB_WeightBlend"},
             {"name": "Dirt", "blend_type": "LB_WeightBlend"},
             {"name": "Rock", "blend_type": "LB_WeightBlend"}])
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        response = unreal.send_command("configure_landscape_layer_blend", {
            "material_path": material_path,
            "expression_id": expression_id,
            "layers": layers
        })
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"configure_landscape_layer_blend error: {e}")
        return {"success": False, "message": str(e)}


# ============================================================================
# LANDSCAPE / TERRAIN TOOLS
# ============================================================================

@mcp.tool()
def get_landscape_info() -> Dict[str, Any]:
    """
    Get information about all landscapes in the current level.

    Returns:
        Dictionary containing:
        - landscapes: List of landscape info including bounds, scale, material, and layers
        - count: Number of landscapes found
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        response = unreal.send_command("get_landscape_info", {})
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"get_landscape_info error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def sculpt_landscape(
    location: List[float],
    radius: float = 500.0,
    strength: float = 0.5,
    raise_terrain: bool = True,
    falloff: float = 0.5
) -> Dict[str, Any]:
    """
    Sculpt (raise or lower) terrain at a world location.

    Parameters:
    - location: World location [X, Y, Z] - Z is ignored, terrain height at XY is modified
    - radius: Brush radius in world units (default: 500)
    - strength: Sculpting strength 0.0-1.0 (default: 0.5)
    - raise_terrain: True to raise, False to lower (default: True)
    - falloff: Falloff curve 0.0-1.0 (default: 0.5, higher = sharper edge)

    Returns:
        Dictionary with success status and modification details.
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        response = unreal.send_command("sculpt_landscape", {
            "location": location,
            "radius": radius,
            "strength": strength,
            "raise": raise_terrain,
            "falloff": falloff
        })
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"sculpt_landscape error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def smooth_landscape(
    location: List[float],
    radius: float = 500.0,
    strength: float = 0.5,
    iterations: int = 1
) -> Dict[str, Any]:
    """
    Smooth terrain at a world location.

    Parameters:
    - location: World location [X, Y, Z]
    - radius: Brush radius in world units (default: 500)
    - strength: Smoothing strength 0.0-1.0 (default: 0.5)
    - iterations: Number of smoothing passes 1-10 (default: 1)

    Returns:
        Dictionary with success status.
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        response = unreal.send_command("smooth_landscape", {
            "location": location,
            "radius": radius,
            "strength": strength,
            "iterations": iterations
        })
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"smooth_landscape error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def flatten_landscape(
    location: List[float],
    radius: float = 500.0,
    strength: float = 1.0,
    target_height: float = None
) -> Dict[str, Any]:
    """
    Flatten terrain at a world location to a specific height.

    Parameters:
    - location: World location [X, Y, Z]
    - radius: Brush radius in world units (default: 500)
    - strength: Flattening strength 0.0-1.0 (default: 1.0)
    - target_height: Target height to flatten to (default: None, uses height at center)

    Returns:
        Dictionary with success status.
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        params = {
            "location": location,
            "radius": radius,
            "strength": strength
        }
        if target_height is not None:
            params["target_height"] = target_height

        response = unreal.send_command("flatten_landscape", params)
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"flatten_landscape error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def paint_landscape_layer(
    location: List[float],
    layer_name: str,
    radius: float = 500.0,
    strength: float = 1.0,
    falloff: float = 0.5
) -> Dict[str, Any]:
    """
    Paint a material layer on the terrain at a world location.

    Parameters:
    - location: World location [X, Y, Z]
    - layer_name: Name of the landscape layer to paint (e.g., "Grass", "Dirt", "Rock")
    - radius: Brush radius in world units (default: 500)
    - strength: Paint strength 0.0-1.0 (default: 1.0)
    - falloff: Falloff curve 0.0-1.0 (default: 0.5)

    Returns:
        Dictionary with success status.

    Note: Use get_landscape_layers() first to see available layers.
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        response = unreal.send_command("paint_landscape_layer", {
            "location": location,
            "layer_name": layer_name,
            "radius": radius,
            "strength": strength,
            "falloff": falloff
        })
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"paint_landscape_layer error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def get_landscape_layers(
    landscape_name: str = ""
) -> Dict[str, Any]:
    """
    Get available paint layers for a landscape.

    Parameters:
    - landscape_name: Optional name of specific landscape (default: first found)

    Returns:
        Dictionary containing list of available layers with names and paths.
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        response = unreal.send_command("get_landscape_layers", {
            "landscape_name": landscape_name
        })
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"get_landscape_layers error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def set_landscape_material(
    material_path: str,
    landscape_name: str = ""
) -> Dict[str, Any]:
    """
    Set the material for a landscape.

    Parameters:
    - material_path: Path to the landscape material (e.g., "/Game/Materials/M_Landscape")
    - landscape_name: Optional name of specific landscape (default: first found)

    Returns:
        Dictionary with success status.
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        response = unreal.send_command("set_landscape_material", {
            "material_path": material_path,
            "landscape_name": landscape_name
        })
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"set_landscape_material error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def create_landscape_layer(
    layer_name: str,
    save_path: str = "/Game/Landscape/Layers"
) -> Dict[str, Any]:
    """
    Create a ULandscapeLayerInfoObject asset for use in landscape material painting.

    This creates a layer info object that can be assigned to a landscape material's layer
    for weight-based texture blending. Use this before paint_landscape_layer.

    Parameters:
    - layer_name: Name of the layer (e.g., "Grass", "Dirt", "Rock")
    - save_path: Content browser path to save the asset (default: /Game/Landscape/Layers)

    Returns:
        Dictionary with success status and path to created layer info asset.

    Workflow:
    1. Create a landscape material with multiple layer blend nodes
    2. Call create_landscape_layer() for each layer name used in your material
    3. Call add_layer_to_landscape() to assign each layer to your landscape
    4. Call paint_landscape_layer() to paint the layers on the terrain
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        response = unreal.send_command("create_landscape_layer", {
            "layer_name": layer_name,
            "save_path": save_path
        })
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"create_landscape_layer error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def add_layer_to_landscape(
    layer_path: str,
    landscape_name: str = ""
) -> Dict[str, Any]:
    """
    Add a landscape layer info object to a landscape for painting.

    After creating a layer with create_landscape_layer(), use this to register
    it with a specific landscape so it can be painted using paint_landscape_layer().

    Parameters:
    - layer_path: Asset path to the layer info (e.g., "/Game/Landscape/Layers/Grass")
    - landscape_name: Optional name of specific landscape (default: first found)

    Returns:
        Dictionary with success status.

    Workflow:
    1. create_landscape_layer("Grass") -> creates /Game/Landscape/Layers/Grass
    2. add_layer_to_landscape("/Game/Landscape/Layers/Grass") -> registers with landscape
    3. paint_landscape_layer("Grass", x, y, radius, strength) -> paints the layer
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    try:
        response = unreal.send_command("add_layer_to_landscape", {
            "layer_path": layer_path,
            "landscape_name": landscape_name
        })
        return response or {"success": False, "message": "No response from Unreal"}
    except Exception as e:
        logger.error(f"add_layer_to_landscape error: {e}")
        return {"success": False, "message": str(e)}


@mcp.tool()
def scatter_foliage(
    mesh_path: str,
    center: List[float] = None,
    radius: float = 0,
    count: int = 100,
    min_distance: float = 50.0,
    max_slope: float = 30.0,
    align_to_surface: bool = False,
    random_yaw: bool = True,
    scale_range: List[float] = None,
    z_offset: float = 0.0,
    actor_name: str = "HISM_Foliage",
    cull_distance: float = 0.0,
    material_path: str = "",
    materials: List[str] = None,
    bounds: List[float] = None
) -> Dict[str, Any]:
    """
    Scatter vegetation/foliage using HISM (HierarchicalInstancedStaticMesh) with
    Poisson disk distribution and automatic slope filtering.

    Uses grid-accelerated dart-throwing for natural, non-overlapping placement.
    Line traces determine terrain height and slope at each point.
    All instances are batched into a single HISM component for optimal performance.

    Parameters:
    - mesh_path: UStaticMesh asset path (e.g., "/Game/Meshes/Vegetation/Grass/SM_Grass_01")
    - center: World XY center [X, Y] (required if bounds not provided)
    - radius: Scatter radius in Unreal units (required if bounds not provided)
    - count: Target instance count (default: 100, max: 50000)
    - min_distance: Minimum distance between instances (default: 50)
    - max_slope: Maximum terrain slope in degrees for placement (default: 30)
    - align_to_surface: Align instance Z-axis to terrain normal (default: false)
    - random_yaw: Apply random yaw rotation to each instance (default: true)
    - scale_range: [min, max] uniform scale range (default: [1, 1])
    - z_offset: Vertical offset from ground in UU (negative = sink into ground)
    - actor_name: Name for the container actor (default: "HISM_Foliage")
    - cull_distance: Instance culling distance in UU (0 = no culling)
    - material_path: Optional material override path (applies to ALL slots)
    - materials: Optional list of material paths, one per slot index.
      Empty strings skip that slot (use mesh default). Overrides material_path if provided.
    - bounds: Optional rectangular bounds [min_x, max_x, min_y, max_y]. When provided,
      overrides center+radius for uniform rectangular coverage. Ideal for full-landscape scatter.

    Returns:
        Dictionary with instance_count, candidates_generated, rejected_slope,
        rejected_no_hit, actor_name, and status message.

    Example usage (circular):
        scatter_foliage(
            mesh_path="/Game/Meshes/Vegetation/Grass/SM_Grass_Large_A",
            center=[-12600, 12600],
            radius=10000,
            count=2000,
            min_distance=100,
            max_slope=15,
            scale_range=[0.6, 1.4],
            z_offset=-3,
            actor_name="HISM_Grass_Large_A"
        )

    Example usage (rectangular bounds for full landscape):
        scatter_foliage(
            mesh_path="/Game/Meshes/Vegetation/Grass/SM_Grass_Large_A",
            bounds=[-25200, 0, 0, 25200],
            count=5000,
            min_distance=80,
            max_slope=45,
            scale_range=[5, 10],
            z_offset=-5,
            actor_name="HISM_Grass_Large_A"
        )
    """
    unreal = get_unreal_connection()
    if not unreal:
        return {"success": False, "message": "Failed to connect to Unreal Engine"}

    # Validate: need either bounds or center+radius
    if bounds is None and center is None:
        return {"success": False, "message": "Must provide either 'bounds' or 'center'+'radius'"}

    params = {
        "mesh_path": mesh_path,
        "count": count,
        "min_distance": min_distance,
        "max_slope": max_slope,
        "align_to_surface": align_to_surface,
        "random_yaw": random_yaw,
        "z_offset": z_offset,
        "actor_name": actor_name,
        "cull_distance": cull_distance,
    }

    if bounds is not None:
        params["bounds"] = bounds
        # Compute center for C++ (it'll be overridden but required by parser)
        if center is None:
            center = [(bounds[0] + bounds[1]) / 2, (bounds[2] + bounds[3]) / 2]
        if radius == 0:
            half_w = (bounds[1] - bounds[0]) / 2
            half_h = (bounds[3] - bounds[2]) / 2
            radius = (half_w**2 + half_h**2) ** 0.5

    params["center"] = center
    params["radius"] = radius

    if scale_range is not None:
        params["scale_range"] = scale_range

    if materials:
        params["materials"] = materials
    elif material_path:
        params["material_path"] = material_path

    try:
        response = unreal.send_command("scatter_foliage", params)
        return response.get("result", response)
    except Exception as e:
        logger.error(f"scatter_foliage error: {e}")
        return {"success": False, "message": str(e)}



# ============================================================================
# Gameplay Commands (FEATURE-017, 018, 020, 022, 023)
# ============================================================================

@mcp.tool()
def set_game_mode_default_pawn(
    blueprint_path: str,
    game_mode_path: str = "",
    create_player_start: bool = True,
    player_start_location: List[float] = [0.0, 0.0, 100.0]
) -> Dict[str, Any]:
    """
    Set the default pawn class for the game mode to a character Blueprint.

    Creates a GameMode BP if none exists, sets DefaultPawnClass,
    and optionally spawns a PlayerStart actor.

    Parameters:
    - blueprint_path: Content path to the character Blueprint (e.g., "/Game/Characters/Robot/BP_RobotCharacter")
    - game_mode_path: Optional path to existing GameMode BP (creates new if empty)
    - create_player_start: Whether to spawn a PlayerStart actor (default: True)
    - player_start_location: [X, Y, Z] for PlayerStart placement (default: [0, 0, 100])
    """
    unreal = get_unreal_connection()
    params = {"blueprint_path": blueprint_path}
    if game_mode_path:
        params["game_mode_path"] = game_mode_path
    params["create_player_start"] = create_player_start
    params["player_start_location"] = player_start_location
    try:
        response = unreal.send_command("set_game_mode_default_pawn", params)
        return response.get("result", response)
    except Exception as e:
        return {"success": False, "message": str(e)}


@mcp.tool()
def create_anim_montage(
    animation_path: str,
    montage_name: str,
    destination_path: str = "",
    slot_name: str = "DefaultGroup.DefaultSlot"
) -> Dict[str, Any]:
    """
    Create an AnimMontage asset from an existing AnimSequence.

    Parameters:
    - animation_path: Content path to source AnimSequence (e.g., "/Game/Characters/Robot/Animations/Anim_Kick")
    - montage_name: Name for the new montage (e.g., "AM_Kick")
    - destination_path: Content path for the montage (defaults to same directory as animation)
    - slot_name: Animation slot name (default: "DefaultGroup.DefaultSlot")
    """
    unreal = get_unreal_connection()
    params = {"animation_path": animation_path, "montage_name": montage_name}
    if destination_path:
        params["destination_path"] = destination_path
    params["slot_name"] = slot_name
    try:
        response = unreal.send_command("create_anim_montage", params)
        return response.get("result", response)
    except Exception as e:
        return {"success": False, "message": str(e)}


@mcp.tool()
def play_montage_on_actor(
    actor_name: str,
    montage_path: str,
    play_rate: float = 1.0,
    start_section: str = ""
) -> Dict[str, Any]:
    """
    Play an AnimMontage on a character actor (requires PIE/Play mode).

    Parameters:
    - actor_name: Name of the character actor in the level
    - montage_path: Content path to the AnimMontage asset
    - play_rate: Playback speed multiplier (default: 1.0)
    - start_section: Optional montage section to start from
    """
    unreal = get_unreal_connection()
    params = {"actor_name": actor_name, "montage_path": montage_path, "play_rate": play_rate}
    if start_section:
        params["start_section"] = start_section
    try:
        response = unreal.send_command("play_montage_on_actor", params)
        return response.get("result", response)
    except Exception as e:
        return {"success": False, "message": str(e)}


@mcp.tool()
def apply_impulse(
    actor_name: str,
    direction: List[float],
    magnitude: float,
    enable_ragdoll: bool = False,
    component_name: str = ""
) -> Dict[str, Any]:
    """
    Apply a physics impulse to an actor. Works best during PIE/Play mode.

    Parameters:
    - actor_name: Name of the target actor
    - direction: [X, Y, Z] direction vector (will be normalized)
    - magnitude: Force magnitude in Unreal units
    - enable_ragdoll: If True and actor is a Character, enables ragdoll physics first (default: False)
    - component_name: Optional specific component to apply impulse to
    """
    unreal = get_unreal_connection()
    params = {
        "actor_name": actor_name,
        "direction": direction,
        "magnitude": magnitude,
        "enable_ragdoll": enable_ragdoll
    }
    if component_name:
        params["component_name"] = component_name
    try:
        response = unreal.send_command("apply_impulse", params)
        return response.get("result", response)
    except Exception as e:
        return {"success": False, "message": str(e)}


@mcp.tool()
def trigger_post_process_effect(
    effect_type: str,
    duration: float = 0.5,
    intensity: float = 1.0,
    custom_settings: Optional[Dict[str, Any]] = None
) -> Dict[str, Any]:
    """
    Trigger a temporary post-process effect (best in PIE/Play mode).

    Parameters:
    - effect_type: "red_flash" (tint screen red), "slow_mo" (slow motion via time dilation),
                   "desaturate" (remove color), "custom" (use custom_settings)
    - duration: How long the effect lasts in seconds (default: 0.5)
    - intensity: Effect strength 0.0-1.0 (default: 1.0)
    - custom_settings: For "custom" type - dict of PostProcess settings to override
    """
    unreal = get_unreal_connection()
    params = {"effect_type": effect_type, "duration": duration, "intensity": intensity}
    if custom_settings:
        params["custom_settings"] = custom_settings
    try:
        response = unreal.send_command("trigger_post_process_effect", params)
        return response.get("result", response)
    except Exception as e:
        return {"success": False, "message": str(e)}


@mcp.tool()
def spawn_niagara_system(
    actor_name: str,
    system_path: str,
    location: List[float] = [0.0, 0.0, 0.0],
    rotation: List[float] = [0.0, 0.0, 0.0],
    scale: List[float] = [1.0, 1.0, 1.0],
    auto_activate: bool = True
) -> Dict[str, Any]:
    """
    Spawn a Niagara particle system actor in the level.

    Parameters:
    - actor_name: Unique name for the actor
    - system_path: Content path to the UNiagaraSystem asset (e.g., "/Game/FX/NS_Fire")
    - location: [X, Y, Z] position
    - rotation: [Pitch, Yaw, Roll] in degrees
    - scale: [X, Y, Z] scale factors
    - auto_activate: Whether particles start automatically (default: True)
    """
    unreal = get_unreal_connection()
    params = {
        "actor_name": actor_name,
        "system_path": system_path,
        "location": location,
        "rotation": rotation,
        "scale": scale,
        "auto_activate": auto_activate
    }
    try:
        response = unreal.send_command("spawn_niagara_system", params)
        return response.get("result", response)
    except Exception as e:
        return {"success": False, "message": str(e)}


@mcp.tool()
def create_niagara_system(
    system_name: str,
    destination_path: str = "/Game/FX",
    template_emitter_path: str = "/Niagara/DefaultAssets/Templates/Emitters/HangingParticulates"
) -> Dict[str, Any]:
    """
    Create a new Niagara particle system asset from a built-in template emitter.

    Duplicates an existing template emitter (e.g., HangingParticulates) into a new
    UNiagaraSystem asset. The system is compiled and saved, ready to be spawned
    with spawn_niagara_system and customized with set_niagara_parameter.

    Parameters:
    - system_name: Name for the new system asset (e.g., "NS_FloatingDust")
    - destination_path: Content directory to create the asset in (default: "/Game/FX")
    - template_emitter_path: Content path to the template emitter to copy
      (default: "/Niagara/DefaultAssets/Templates/Emitters/HangingParticulates")
    """
    unreal = get_unreal_connection()
    params = {
        "system_name": system_name,
        "destination_path": destination_path,
        "template_emitter_path": template_emitter_path
    }
    try:
        response = unreal.send_command("create_niagara_system", params)
        return response.get("result", response)
    except Exception as e:
        return {"success": False, "message": str(e)}


@mcp.tool()
def set_niagara_parameter(
    actor_name: str,
    parameter_name: str,
    parameter_type: str,
    value: Any = None
) -> Dict[str, Any]:
    """
    Set a runtime parameter on a spawned NiagaraActor's Niagara component.

    Allows customizing particle behavior by setting user-exposed parameters
    (e.g., spawn rate, color, size) on a Niagara system that is already placed in the level.

    Parameters:
    - actor_name: Name of the ANiagaraActor in the level
    - parameter_name: Full parameter name (e.g., "User.SpawnRate", "User.Color")
    - parameter_type: One of: "float", "int", "bool", "vector", "vector2d", "position", "color"
    - value: The value to set. Format depends on parameter_type:
      - float/int: plain number (e.g., 100.0)
      - bool: true/false
      - vector/position: [X, Y, Z] array (e.g., [1.0, 0.5, 0.0])
      - vector2d: [X, Y] array (e.g., [1.0, 0.5])
      - color: {"R": 0.72, "G": 0.55, "B": 0.27, "A": 0.4} object (A defaults to 1.0)
    """
    unreal = get_unreal_connection()
    params = {
        "actor_name": actor_name,
        "parameter_name": parameter_name,
        "parameter_type": parameter_type,
        "value": value
    }
    try:
        response = unreal.send_command("set_niagara_parameter", params)
        return response.get("result", response)
    except Exception as e:
        return {"success": False, "message": str(e)}


@mcp.tool()
def create_atmospheric_fx(
    system_name: str,
    preset: str,
    destination_path: str = "/Game/FX"
) -> Dict[str, Any]:
    """
    Create a Niagara particle system with the correct module stack for atmospheric effects.

    Unlike create_niagara_system (which copies a template as-is), this tool builds the proper
    module stack for the requested effect type. The system is ready to spawn and the user can
    fine-tune parameter values in the Niagara editor.

    Presets and their module stacks:
    - "sandstorm": SpawnRate + InitializeParticle + BoxLocation + AddVelocity + CurlNoiseForce + Drag + GravityForce + SolveForcesAndVelocity
    - "ground_mist": SpawnRate + InitializeParticle + BoxLocation + AddVelocity + CurlNoiseForce + Drag + SolveForcesAndVelocity
    - "floating_dust": SpawnRate + InitializeParticle + BoxLocation + CurlNoiseForce + GravityForce + SolveForcesAndVelocity

    Parameters:
    - system_name: Name for the new system asset (e.g., "NS_SandStorm_v2")
    - preset: One of "sandstorm", "ground_mist", "floating_dust"
    - destination_path: Content directory (default: "/Game/FX")
    """
    unreal = get_unreal_connection()
    params = {
        "system_name": system_name,
        "preset": preset,
        "destination_path": destination_path
    }
    try:
        response = unreal.send_command("create_atmospheric_fx", params)
        return response.get("result", response)
    except Exception as e:
        return {"success": False, "message": str(e)}


# ============================================================================
# Skeletal Animation on Placed Actors
# ============================================================================

@mcp.tool()
def set_skeletal_animation(
    actor_name: str,
    animation_path: str,
    looping: bool = True,
    play_rate: float = 1.0,
    component_name: str = ""
) -> Dict[str, Any]:
    """
    Set a looping animation on a placed actor's SkeletalMeshComponent.

    Uses AnimationSingleNode mode with OverrideAnimationData() — no AnimBP needed.
    Works on ACharacter subclasses (auto-finds CharacterMesh0) and any actor
    with a SkeletalMeshComponent.

    Parameters:
    - actor_name: The name of the actor in the level
    - animation_path: Content path to AnimSequence (e.g., "/Game/Characters/MyChar/Anim_Idle")
    - looping: Whether the animation loops (default: True)
    - play_rate: Playback speed multiplier (default: 1.0)
    - component_name: Optional specific SkeletalMeshComponent name (default: auto-detect)
    """
    unreal = get_unreal_connection()
    params = {
        "actor_name": actor_name,
        "animation_path": animation_path,
        "looping": looping,
        "play_rate": play_rate,
    }
    if component_name:
        params["component_name"] = component_name
    try:
        response = unreal.send_command("set_skeletal_animation", params)
        return response.get("result", response)
    except Exception as e:
        return {"success": False, "message": str(e)}


# ============================================================================
# Widget Commands (FEATURE-019)
# ============================================================================

@mcp.tool()
def create_widget_blueprint(
    widget_name: str,
    widget_path: str = "/Game/UI/",
    elements: Optional[List[Dict[str, Any]]] = None
) -> Dict[str, Any]:
    """
    Create a UMG Widget Blueprint with optional child elements.

    Parameters:
    - widget_name: Name for the widget BP (e.g., "WBP_HUD")
    - widget_path: Content browser destination path (default: "/Game/UI/")
    - elements: Optional list of widget elements to add. Each element:
        {"type": "ProgressBar"|"Image"|"TextBlock"|"Border",
         "name": "HealthBar",
         "position": [x, y],
         "size": [width, height],
         "properties": {"Percent": 0.75, "FillColor": [1,0,0,1]}}

    Example:
        create_widget_blueprint("WBP_HUD", elements=[
            {"type": "ProgressBar", "name": "HealthBar", "position": [50, 50], "size": [300, 30]},
            {"type": "TextBlock", "name": "ScoreText", "position": [50, 100], "size": [200, 40]}
        ])
    """
    unreal = get_unreal_connection()
    params = {"widget_name": widget_name, "widget_path": widget_path}
    if elements:
        params["elements"] = elements
    try:
        response = unreal.send_command("create_widget_blueprint", params)
        return response.get("result", response)
    except Exception as e:
        return {"success": False, "message": str(e)}


@mcp.tool()
def add_widget_to_viewport(
    widget_path: str,
    z_order: int = 0
) -> Dict[str, Any]:
    """
    Add a Widget Blueprint to the viewport. Works during PIE; in editor mode, validates the asset.

    Parameters:
    - widget_path: Content path to the Widget Blueprint
    - z_order: Viewport Z-order (higher = on top, default: 0)
    """
    unreal = get_unreal_connection()
    params = {"widget_path": widget_path, "z_order": z_order}
    try:
        response = unreal.send_command("add_widget_to_viewport", params)
        return response.get("result", response)
    except Exception as e:
        return {"success": False, "message": str(e)}


@mcp.tool()
def set_widget_property(
    widget_path: str,
    widget_name: str,
    property_name: str,
    value: Any
) -> Dict[str, Any]:
    """
    Set a property on a named child widget inside a Widget Blueprint.

    Parameters:
    - widget_path: Content path to the Widget Blueprint
    - widget_name: Name of the child widget (e.g., "HealthBar")
    - property_name: Property to set. Common properties:
        ProgressBar: "Percent" (0-1), "FillColor" ([R,G,B,A])
        TextBlock: "Text" (string), "FontSize" (int), "ColorAndOpacity" ([R,G,B,A])
        Image: "ColorAndOpacity" ([R,G,B,A]), "Visibility" ("Visible"|"Hidden"|"Collapsed")
    - value: The value to set (type depends on property)
    """
    unreal = get_unreal_connection()
    params = {
        "widget_path": widget_path,
        "widget_name": widget_name,
        "property_name": property_name,
        "value": value
    }
    try:
        response = unreal.send_command("set_widget_property", params)
        return response.get("result", response)
    except Exception as e:
        return {"success": False, "message": str(e)}


# ============================================================================
# AI Commands (FEATURE-021)
# ============================================================================

@mcp.tool()
def create_behavior_tree(
    bt_name: str,
    bt_path: str = "/Game/AI/",
    root_type: str = "Selector"
) -> Dict[str, Any]:
    """
    Create a Behavior Tree asset with a root composite node.

    Parameters:
    - bt_name: Name for the BT (e.g., "BT_EnemyPatrol")
    - bt_path: Content browser destination path (default: "/Game/AI/")
    - root_type: Root composite type - "Selector" or "Sequence" (default: "Selector")
    """
    unreal = get_unreal_connection()
    params = {"bt_name": bt_name, "bt_path": bt_path, "root_type": root_type}
    try:
        response = unreal.send_command("create_behavior_tree", params)
        return response.get("result", response)
    except Exception as e:
        return {"success": False, "message": str(e)}


@mcp.tool()
def create_blackboard(
    bb_name: str,
    bb_path: str = "/Game/AI/",
    keys: Optional[List[Dict[str, str]]] = None
) -> Dict[str, Any]:
    """
    Create a Blackboard Data asset with typed keys.

    Parameters:
    - bb_name: Name for the Blackboard (e.g., "BB_EnemyData")
    - bb_path: Content browser destination path (default: "/Game/AI/")
    - keys: List of key definitions, each: {"name": "TargetActor", "type": "Object"|"Bool"|"Int"|"Float"|"Vector"|"String"}

    Example:
        create_blackboard("BB_EnemyData", keys=[
            {"name": "TargetActor", "type": "Object"},
            {"name": "PatrolIndex", "type": "Int"},
            {"name": "IsAlerted", "type": "Bool"}
        ])
    """
    unreal = get_unreal_connection()
    params = {"bb_name": bb_name, "bb_path": bb_path}
    if keys:
        params["keys"] = keys
    try:
        response = unreal.send_command("create_blackboard", params)
        return response.get("result", response)
    except Exception as e:
        return {"success": False, "message": str(e)}


@mcp.tool()
def add_bt_task(
    bt_path: str,
    task_type: str,
    task_params: Optional[Dict[str, Any]] = None
) -> Dict[str, Any]:
    """
    Add a task node to a Behavior Tree's root composite.

    Parameters:
    - bt_path: Content path to the Behavior Tree asset
    - task_type: Task type - "MoveTo", "Wait", "PlayAnimation", "RunEQSQuery"
    - task_params: Optional task-specific settings:
        MoveTo: {"acceptable_radius": 100.0}
        Wait: {"wait_time": 3.0}
        PlayAnimation: {"animation_path": "/Game/..."}
    """
    unreal = get_unreal_connection()
    params = {"bt_path": bt_path, "task_type": task_type}
    if task_params:
        params["task_params"] = task_params
    try:
        response = unreal.send_command("add_bt_task", params)
        return response.get("result", response)
    except Exception as e:
        return {"success": False, "message": str(e)}


@mcp.tool()
def add_bt_decorator(
    bt_path: str,
    decorator_type: str,
    child_index: int = 0,
    decorator_params: Optional[Dict[str, Any]] = None
) -> Dict[str, Any]:
    """
    Add a decorator to a Behavior Tree child node.

    Parameters:
    - bt_path: Content path to the Behavior Tree asset
    - decorator_type: "Blackboard", "Cooldown", "TimeLimit", "IsAtLocation"
    - child_index: Index of the child node to decorate (default: 0)
    - decorator_params: Optional decorator settings:
        Cooldown: {"cooldown_time": 5.0}
        TimeLimit: {"time_limit": 10.0}
        Blackboard: {"blackboard_key": "IsAlerted", "key_query": "IsSet"}
    """
    unreal = get_unreal_connection()
    params = {"bt_path": bt_path, "decorator_type": decorator_type, "child_index": child_index}
    if decorator_params:
        params["decorator_params"] = decorator_params
    try:
        response = unreal.send_command("add_bt_decorator", params)
        return response.get("result", response)
    except Exception as e:
        return {"success": False, "message": str(e)}


@mcp.tool()
def assign_behavior_tree(
    actor_name: str,
    bt_path: str,
    bb_path: str = ""
) -> Dict[str, Any]:
    """
    Assign a Behavior Tree to an AI-controlled actor. Works in PIE; in editor provides guidance.

    Parameters:
    - actor_name: Name of the NPC actor (must be a Pawn with AIController)
    - bt_path: Content path to the Behavior Tree asset
    - bb_path: Optional content path to a Blackboard Data asset
    """
    unreal = get_unreal_connection()
    params = {"actor_name": actor_name, "bt_path": bt_path}
    if bb_path:
        params["bb_path"] = bb_path
    try:
        response = unreal.send_command("assign_behavior_tree", params)
        return response.get("result", response)
    except Exception as e:
        return {"success": False, "message": str(e)}


def main():
    """Entry point for the it-is-unreal MCP server."""
    logger.info("Starting it-is-unreal MCP server with stdio transport")
    mcp.run(transport='stdio')


# Run the server
if __name__ == "__main__":
    main()
