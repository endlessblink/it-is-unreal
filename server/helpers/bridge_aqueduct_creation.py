"""
Bridge and Aqueduct Creation Helpers

Contains helper functions for creating suspension bridges and aqueducts with
dynamic parameters, safe spawning, and performance metrics.
"""

import math
import logging
from typing import List, Dict, Any, Tuple
from .actor_name_manager import safe_spawn_actor

logger = logging.getLogger("BridgeAqueductCreation")


def calculate_parabolic_cable_points(
    span_length: float,
    sag_ratio: float,
    tower_height: float,
    module_size: float,
    start_location: List[float]
) -> List[Tuple[List[float], float]]:
    """
    Calculate points along a parabolic cable for a suspension bridge.
    Returns list of (location, angle) tuples for cable segments.
    """
    points = []

    # Parabola equation: y = ax^2 + c
    # At x=0 (center): y = -sag
    # At x=±span/2: y = 0 (tower top)
    sag = span_length * sag_ratio
    a = 4 * sag / (span_length ** 2)

    # Sample points along the span
    num_segments = max(1, int(span_length / module_size))

    for i in range(num_segments + 1):
        # X position from -span/2 to +span/2
        x = -span_length/2 + (i * span_length / num_segments)

        # Parabolic height relative to tower top
        y_relative = a * x * x - sag

        # World position
        world_x = start_location[0] + x
        world_y = start_location[1]
        world_z = start_location[2] + tower_height + y_relative

        # Calculate angle for this segment (tangent to parabola)
        if i < num_segments:
            next_x = -span_length/2 + ((i + 1) * span_length / num_segments)
            next_y_relative = a * next_x * next_x - sag

            dx = next_x - x
            dy = next_y_relative - y_relative
            angle = math.degrees(math.atan2(dy, dx))
        else:
            angle = 0  # Last point

        points.append(([world_x, world_y, world_z], angle))

    return points


def build_suspension_bridge_structure(
    unreal,
    span_length: float,
    deck_width: float,
    tower_height: float,
    cable_sag_ratio: float,
    module_size: float,
    location: List[float],
    orientation: str,
    name_prefix: str,
    deck_mesh: str,
    tower_mesh: str,
    cable_mesh: str,
    suspender_mesh: str,
    all_actors: List[Dict[str, Any]]
) -> Dict[str, int]:
    """Build all components of a suspension bridge."""
    logger.info(f"Starting bridge construction: {name_prefix}, span={span_length}, width={deck_width}")
    counts = {
        "towers": 0,
        "deck_segments": 0,
        "cable_segments": 0,
        "suspenders": 0
    }

    # Adjust for orientation
    if orientation == "y":
        # Swap X and Y for Y-oriented bridge
        location_adjusted = location.copy()
        span_direction = 1  # Y-axis
    else:
        location_adjusted = location.copy()
        span_direction = 0  # X-axis

    # Two main cables (one on each side) - define early for tower attachments
    cable_offsets = [-deck_width/2, deck_width/2]

    # Build towers
    tower_positions = [
        [-span_length/2, 0, 0],
        [span_length/2, 0, 0]
    ]

    for i, tower_pos in enumerate(tower_positions):
        tower_location = location_adjusted.copy()
        if span_direction == 0:
            tower_location[0] += tower_pos[0]
        else:
            tower_location[1] += tower_pos[0]

        # Tower consists of multiple segments for better visual
        # Tower base (wider foundation) - starts at surface level
        base_params = {
            "name": f"{name_prefix}_Tower_{i}_Base",
            "type": "StaticMeshActor",
            "location": [tower_location[0], tower_location[1], tower_location[2] + 200],
            "scale": [5.0, 5.0, 4.0],
            "static_mesh": tower_mesh
        }
        resp = safe_spawn_actor(unreal, base_params)
        if resp and resp.get("status") == "success":
            all_actors.append(resp)
            counts["towers"] += 1

        # Main tower shaft (very tall) - positioned above the base
        main_shaft_params = {
            "name": f"{name_prefix}_Tower_{i}_Main",
            "type": "StaticMeshActor",
            "location": [tower_location[0], tower_location[1], tower_location[2] + 400 + tower_height/2],
            "scale": [3.0, 3.0, tower_height/100],
            "static_mesh": tower_mesh
        }
        resp = safe_spawn_actor(unreal, main_shaft_params)
        if resp and resp.get("status") == "success":
            all_actors.append(resp)
            counts["towers"] += 1

        # Tower top cap where cables attach
        top_location = tower_location.copy()
        top_location[2] += 400 + tower_height

        top_params = {
            "name": f"{name_prefix}_Tower_{i}_Top",
            "type": "StaticMeshActor",
            "location": top_location,
            "scale": [3.5, 3.5, 1.0],
            "static_mesh": tower_mesh
        }
        resp = safe_spawn_actor(unreal, top_params)
        if resp and resp.get("status") == "success":
            all_actors.append(resp)
            counts["towers"] += 1

        # Add cable attachment points (small blocks at tower top)
        for side_offset in cable_offsets:
            attachment_location = top_location.copy()
            if span_direction == 0:
                attachment_location[1] += side_offset
            else:
                attachment_location[0] += side_offset

            attachment_params = {
                "name": f"{name_prefix}_Tower_{i}_Attachment_{int(side_offset)}",
                "type": "StaticMeshActor",
                "location": attachment_location,
                "scale": [0.5, 0.5, 0.5],
                "static_mesh": tower_mesh
            }
            resp = safe_spawn_actor(unreal, attachment_params)
            if resp and resp.get("status") == "success":
                all_actors.append(resp)
                counts["towers"] += 1

    # Build main cables - adjust for new tower positioning (400 units above ground + tower_height)
    effective_tower_height = 400 + tower_height
    cable_points = calculate_parabolic_cable_points(
        span_length, cable_sag_ratio, effective_tower_height, module_size, location_adjusted
    )

    for cable_idx, offset in enumerate(cable_offsets):
        for i in range(len(cable_points) - 1):
            point, angle = cable_points[i]
            next_point, _ = cable_points[i + 1]

            # Calculate segment length and midpoint
            if span_direction == 0:
                dx = next_point[0] - point[0]
                dy = 0
                cable_location = [
                    (point[0] + next_point[0]) / 2,
                    point[1] + offset,
                    (point[2] + next_point[2]) / 2
                ]
                rotation = [angle, 0, 0]
            else:
                dx = 0
                dy = next_point[0] - point[0]  # Span direction along Y-axis
                cable_location = [
                    location_adjusted[0] + offset,  # Perpendicular to span (X-axis)
                    (point[0] + next_point[0]) / 2,  # Along span (Y-axis) - using X from cable points
                    (point[2] + next_point[2]) / 2
                ]
                rotation = [0, angle, 0]

            dz = next_point[2] - point[2]
            segment_length = math.sqrt(dx*dx + dy*dy + dz*dz)

            cable_params = {
                "name": f"{name_prefix}_Cable_{cable_idx}_{i}",
                "type": "StaticMeshActor",
                "location": cable_location,
                "rotation": rotation,
                "scale": [0.3, 0.3, segment_length/100],
                "static_mesh": cable_mesh
            }
            resp = safe_spawn_actor(unreal, cable_params)
            if resp and resp.get("status") == "success":
                all_actors.append(resp)
                counts["cable_segments"] += 1

    # Build deck
    deck_segments_x = max(1, int(span_length / module_size))
    deck_segments_y = max(1, int(deck_width / module_size))

    for i in range(deck_segments_x):
        for j in range(deck_segments_y):
            if span_direction == 0:
                deck_x = location_adjusted[0] - span_length/2 + (i + 0.5) * module_size
                deck_y = location_adjusted[1] - deck_width/2 + (j + 0.5) * module_size
            else:
                deck_x = location_adjusted[0] - deck_width/2 + (j + 0.5) * module_size
                deck_y = location_adjusted[1] - span_length/2 + (i + 0.5) * module_size

            deck_params = {
                "name": f"{name_prefix}_Deck_{i}_{j}",
                "type": "StaticMeshActor",
                "location": [deck_x, deck_y, location_adjusted[2]],
                "scale": [module_size/100, module_size/100, 0.5],
                "static_mesh": deck_mesh
            }
            resp = safe_spawn_actor(unreal, deck_params)
            if resp and resp.get("status") == "success":
                all_actors.append(resp)
                counts["deck_segments"] += 1

    # Build vertical suspenders
    suspender_spacing = module_size * 3  # Every 3 modules
    num_suspenders = max(1, int(span_length / suspender_spacing))

    for i in range(num_suspenders):
        x_pos = -span_length/2 + (i + 0.5) * suspender_spacing

        # Find cable height at this position
        cable_height_relative = cable_sag_ratio * span_length * (4 * x_pos * x_pos / (span_length * span_length) - 1)
        cable_z = location_adjusted[2] + effective_tower_height + cable_height_relative
        deck_z = location_adjusted[2]

        suspender_height = cable_z - deck_z

        for offset in cable_offsets:
            if span_direction == 0:
                susp_location = [
                    location_adjusted[0] + x_pos,
                    location_adjusted[1] + offset,
                    (cable_z + deck_z) / 2
                ]
            else:
                susp_location = [
                    location_adjusted[0] + offset,
                    location_adjusted[1] + x_pos,
                    (cable_z + deck_z) / 2
                ]

            suspender_params = {
                "name": f"{name_prefix}_Suspender_{i}_{int(offset)}",
                "type": "StaticMeshActor",
                "location": susp_location,
                "scale": [0.1, 0.1, suspender_height/100],
                "static_mesh": suspender_mesh
            }
            resp = safe_spawn_actor(unreal, suspender_params)
            if resp and resp.get("status") == "success":
                all_actors.append(resp)
                counts["suspenders"] += 1

    return counts


def calculate_arch_points(
    arch_radius: float,
    module_size: float,
    pier_width: float,
    arch_index: int,
    tier_height: float,
    location: List[float],
    orientation: str
) -> List[Tuple[List[float], float]]:
    """Calculate points along a semicircular arch."""
    points = []

    # Number of segments based on module size
    circumference = math.pi * arch_radius
    num_segments = max(4, int(circumference / module_size))

    # Arch spans from one pier to the next
    arch_span_x = arch_index * (2 * arch_radius + pier_width) + pier_width/2

    for i in range(num_segments + 1):
        # Angle from 0 to π (semicircle)
        angle = i * math.pi / num_segments

        # Position relative to arch center
        x_rel = arch_radius * math.cos(angle)
        z_rel = arch_radius * math.sin(angle)

        if orientation == "x":
            world_x = location[0] + arch_span_x + arch_radius + x_rel
            world_y = location[1]
        else:
            world_x = location[0]
            world_y = location[1] + arch_span_x + arch_radius + x_rel

        world_z = location[2] + tier_height + z_rel

        # Calculate tangent angle for segment orientation
        if i < num_segments:
            next_angle = (i + 1) * math.pi / num_segments
            dx = math.cos(next_angle) - math.cos(angle)
            dz = math.sin(next_angle) - math.sin(angle)
            tangent_angle = math.degrees(math.atan2(dz, dx))
        else:
            tangent_angle = 90  # Last point

        points.append(([world_x, world_y, world_z], tangent_angle))

    return points


def build_aqueduct_structure(
    unreal,
    arches: int,
    arch_radius: float,
    pier_width: float,
    tiers: int,
    deck_width: float,
    module_size: float,
    location: List[float],
    orientation: str,
    name_prefix: str,
    arch_mesh: str,
    pier_mesh: str,
    deck_mesh: str,
    all_actors: List[Dict[str, Any]]
) -> Dict[str, int]:
    """Build all components of an aqueduct."""
    counts = {
        "arch_segments": 0,
        "piers": 0,
        "deck_segments": 0
    }

    # Calculate dimensions
    arch_spacing = 2 * arch_radius + pier_width
    total_length = arches * arch_spacing + pier_width
    tier_height = 2 * arch_radius + pier_width  # Height of each tier

    # Build tiers from bottom to top
    for tier in range(tiers):
        current_tier_height = tier * tier_height

        # Build piers for this tier
        for pier_idx in range(arches + 1):
            pier_x = pier_idx * arch_spacing

            if orientation == "x":
                pier_location = [
                    location[0] + pier_x,
                    location[1],
                    location[2] + current_tier_height
                ]
            else:
                pier_location = [
                    location[0],
                    location[1] + pier_x,
                    location[2] + current_tier_height
                ]

            # Make piers taper slightly on higher tiers
            pier_scale_factor = 1.0 - (tier * 0.1)  # 10% smaller each tier

            pier_params = {
                "name": f"{name_prefix}_Pier_T{tier}_P{pier_idx}",
                "type": "StaticMeshActor",
                "location": pier_location,
                "scale": [
                    pier_width/100 * pier_scale_factor,
                    pier_width/100 * pier_scale_factor,
                    tier_height/100
                ],
                "static_mesh": pier_mesh
            }
            resp = safe_spawn_actor(unreal, pier_params)
            if resp and resp.get("status") == "success":
                all_actors.append(resp)
                counts["piers"] += 1

        # Build arches for this tier
        for arch_idx in range(arches):
            arch_points = calculate_arch_points(
                arch_radius, module_size, pier_width, arch_idx,
                current_tier_height, location, orientation
            )

            # Create arch segments
            for i in range(len(arch_points) - 1):
                point, angle = arch_points[i]
                next_point, _ = arch_points[i + 1]

                # Calculate segment midpoint and length
                mid_x = (point[0] + next_point[0]) / 2
                mid_y = (point[1] + next_point[1]) / 2
                mid_z = (point[2] + next_point[2]) / 2

                dx = next_point[0] - point[0]
                dy = next_point[1] - point[1]
                dz = next_point[2] - point[2]
                segment_length = math.sqrt(dx*dx + dy*dy + dz*dz)

                # Rotation based on orientation
                if orientation == "x":
                    rotation = [angle, 0, 90]  # Cylinder along arch
                else:
                    rotation = [0, angle, 90]

                arch_params = {
                    "name": f"{name_prefix}_Arch_T{tier}_A{arch_idx}_S{i}",
                    "type": "StaticMeshActor",
                    "location": [mid_x, mid_y, mid_z],
                    "rotation": rotation,
                    "scale": [
                        pier_width/200 * pier_scale_factor,  # Arch thickness
                        pier_width/200 * pier_scale_factor,
                        segment_length/100
                    ],
                    "static_mesh": arch_mesh
                }
                resp = safe_spawn_actor(unreal, arch_params)
                if resp and resp.get("status") == "success":
                    all_actors.append(resp)
                    counts["arch_segments"] += 1

    # Build water deck on top tier
    deck_height = location[2] + tiers * tier_height
    deck_segments_length = max(1, int(total_length / module_size))
    deck_segments_width = max(1, int(deck_width / module_size))

    for i in range(deck_segments_length):
        for j in range(deck_segments_width):
            if orientation == "x":
                deck_x = location[0] + (i + 0.5) * module_size
                deck_y = location[1] - deck_width/2 + (j + 0.5) * module_size
            else:
                deck_x = location[0] - deck_width/2 + (j + 0.5) * module_size
                deck_y = location[1] + (i + 0.5) * module_size

            deck_params = {
                "name": f"{name_prefix}_Deck_{i}_{j}",
                "type": "StaticMeshActor",
                "location": [deck_x, deck_y, deck_height],
                "scale": [module_size/100, module_size/100, 0.5],
                "static_mesh": deck_mesh
            }
            resp = safe_spawn_actor(unreal, deck_params)
            if resp and resp.get("status") == "success":
                all_actors.append(resp)
                counts["deck_segments"] += 1

    # Add side walls to water channel
    for side in [0, 1]:
        for i in range(deck_segments_length):
            if orientation == "x":
                wall_x = location[0] + (i + 0.5) * module_size
                wall_y = location[1] + (deck_width/2 if side else -deck_width/2)
            else:
                wall_x = location[0] + (deck_width/2 if side else -deck_width/2)
                wall_y = location[1] + (i + 0.5) * module_size

            wall_params = {
                "name": f"{name_prefix}_Wall_S{side}_{i}",
                "type": "StaticMeshActor",
                "location": [wall_x, wall_y, deck_height + 100],
                "scale": [module_size/100, 0.2, 2.0],
                "static_mesh": deck_mesh
            }
            resp = safe_spawn_actor(unreal, wall_params)
            if resp and resp.get("status") == "success":
                all_actors.append(resp)
                # Count as deck segments for simplicity
                counts["deck_segments"] += 1

    return counts
