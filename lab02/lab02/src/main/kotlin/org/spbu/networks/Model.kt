package org.spbu.networks

import kotlinx.serialization.Serializable
import java.awt.Image

@Serializable
class Product(
    var id: Int,
    var name: String,
    var description: String,
    var icon: String? = null
)

@Serializable
data class AddQuery(
    val name: String,
    val description: String
)
